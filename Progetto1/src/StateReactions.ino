/*
fase di preparazione del gioco, in caso di sconfitta
Se l'arduino uno non viene resettato il setup non viene rilanciato,
risulta quindi necessaria una ulteriore fase di preparazione
*/
void preparationGame(){
  lcd.clear();
  // reset dati player 1 
  p1.pointsPlayer = 0;
  p1.numClick = 0;
  digitalWrite(p1.pinLedRed, LOW);
  digitalWrite(p1.pinLedGreen, LOW);

  // reset dati player 2
  p2.pointsPlayer = 0;
  p2.numClick = 0;

  digitalWrite(p2.pinLedRed, LOW);
  digitalWrite(p2.pinLedGreen, LOW);
  
  gameStateActual = READY_TO_PLAY;

  stateTimer = millis();
}


/*
Funzione di attesa di un click da parte dle giocatore di uno dei 2 bottoni
In questa fase i led lampeggiano 'fading' e dopo 10 secondi di inattività del giocatore
il sistema passerà in 'SLEEP_MODE' 
*/
void readyToPlay() {
  static int fadingValue1 = 5;             //quantità di luminosità da aggiungere o togliere al led 1
  static int fadingValue2 = 5;             //quantità di luminosità da aggiungere o togliere al led 2
  static int brightLevel1 = 0;              //quanto è luminoso il led 1
  static int brightLevel2 = 255;            //quanto è luminoso il led 2

  if (isFirstRoundOfPlay) {
    potValue = updateDifficultyFromPot();
    digitalWrite(p1.pinLedGreen, LOW);
    digitalWrite(p2.pinLedGreen, LOW);
    isFirstRoundOfPlay = false;
  }

  ledAscending(PIN_RED_1, brightLevel1, fadingValue1);
  ledDescending(PIN_RED_2, brightLevel2, fadingValue2);
  showDisplay("Welcome to the React! Game. Press Buttons to Start");
  // controllo per verificare se è passato abbastanza tempo e si puo' entrare in modalità sleep
  if (currentMillis - stateTimer >= TIME_BEFORE_SLEEPING) {
        gameStateActual = SLEEP_MODE;
    } 
  /*
  controllo che gli utenti abbiano cliccato i pulsanti
  non è necessaria una pressione simultanea.
  */
  if (p1.numClick > 0 && p2.numClick > 0) { 
  // ma se metto && si tratta di condizione di click simultaneo
  // se invece nel click con interrupt introduco un status o controllo con numClick
    gameStateActual = PLAY;
    setupPlay();
  }

}


// esegue il gioco vero e proprio
void play() {
  attachInterrupt(digitalPinToInterrupt(p1.pinBtn), actionCounterPlayer1, RISING);
  attachInterrupt(digitalPinToInterrupt(p2.pinBtn), actionCounterPlayer2, RISING);
  if(!isNumberSpawned) {
    // Attesa prima che compaia il numero

    //Controllo falsa partenza
    if (p1.numClick > 0 || p2.numClick > 0) {
      gameStateActual = CHECK_VICTORY; // Interrompe il round
      detachInterrupt(digitalPinToInterrupt(p1.pinBtn));
      detachInterrupt(digitalPinToInterrupt(p2.pinBtn));
      return; // Esce dalla funzione play
    }

    if(millis() - roundStartTime >= timeToWait) {
      // Il tempo di attesa è scaduto , mostriamo il numero
      isNumberSpawned = true;
      lcd.clear(); // Pulisce lo schermo dal "Go!"

      
      //currentT1 *= potValue ; // qui aggiorno il tempo dato per ogni round con il potenziometro
      currentT1 = currentT1 * (1.0 - potValue) ;
      timerForMatchInGame=millis();
    }
  }else{
    // Il numero è uscito.
    showDisplay(String(numClickRound));
    Serial.println(String(numClickRound));
    /*
    superato il tempo di gioco controllo lo stato dei punteggi
    */
    if(millis()- timerForMatchInGame > currentT1){ // ho sostituito timeToWait

      /*
      disattivo interrupts per non dover gestire click oltre tempo massimo 
      --> non eisistono le penalita' per chi clicca in ritardo, non lo contiamo e basta 
      */
      detachInterrupt(digitalPinToInterrupt(p1.pinBtn));
      detachInterrupt(digitalPinToInterrupt(p2.pinBtn));

      /*
      Se il tempo di attesa è scaduto senza alcun click dei giocatori, chiudo il gioco 
      Status GAME_OVER
      */
      if(p1.numClick == 0 && p2.numClick == 0) {
        lcd.clear(); // Pulisce lo schermo da errori o rimasugli
        gameStateActual = GAME_OVER;
        gameOverTimer= millis();
      }else{
      //almeno un click e' avvenuto, quindi vado avanti nel gioco
      gameStateActual=CHECK_VICTORY;
      }

    }
  }
} 

/*
funzione per lo stato di SLEEP
dopo 10 secondi dall'avvio del gioco
il sistema andra' in modalita' sleep 
se non veranno rilevati click da parte dell'utente
*/
void deepSleepWhileWait(){
  lcd.clear();
  lcd.noBacklight();
  analogWrite(PIN_RED_1, 0);
  analogWrite(PIN_RED_2, 0);
  digitalWrite(PIN_GREEN_1, LOW);
  digitalWrite(PIN_GREEN_2, LOW);
  Serial.flush();
  delay(MIN_WAIT); //uso MIN_WAIT per non usare magic numbers  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  sleep_enable();
  sleep_mode(); 
  // Il microcontrollore si ferma qui fino all'interrupt
  sleep_disable();
  lcd.backlight();
  gameStateActual = PREPARATION;
}


/*
Funzione per il controllo della vittoria.
verifichiamo quale giocatore ha eseguito il numero di click corretto.
se entrambi hanno eseguito un numero di click corretto guardiamo i tempi dell'ultimo click per decretare il vincitore.
Viene preso in considerazione la casistica di pareggio sia per numero di click che per tempistiche dell'ultimo click.

dopo aver verificato le condizioni di vittoria per un giocatore 
e aver applicato i malus al giocatore avversario,  
attiviamo il led verde al vincitore.

i malus vengono applicati solo se il numero di click è errato, 
o se il click viene eseguito prima che venga mostrato il numero.
se invece entrambi i giocatori eseguono il numero di click corretto, 
nessuno dei due avrà malus, solo led attivi, verde al piu veloce e rosso al altro.
*/
void logicForCheckVictory(){
  lcd.clear(); // Pulisco il display
  /*
  si entra qua solo se nel play viene rilevata la violazione di click anticipato
  il flag isNumberSpawned non viene resettato, e cambia lo status in : gameStateActual = CHECK_VICTORY
  */
  if (!isNumberSpawned) {
    Serial.println("Falsa partenza!");
    if (p1.numClick > 0) {
      p1.pointsPlayer--;
      digitalWrite(p1.pinLedRed, HIGH);
    }
    if (p2.numClick > 0) {
      p2.pointsPlayer--;
      digitalWrite(p2.pinLedRed, HIGH);
    }
  }else {
    //CASO 1
    // pareggio click, controllo i tempi
    if(p1.numClick == numClickRound && p2.numClick == numClickRound){
      //i tempi di risposta del ultimo click sono stati identici
      if(p1.lastClick == p2.lastClick){
        Serial.println("Tie");
        //nessun vincitore quindi non incremento i punteggi
      }else if(p1.lastClick < p2.lastClick){
        Serial.println("vince P1");
        p1.pointsPlayer++;
        digitalWrite(p1.pinLedGreen, HIGH);
        //digitalWrite(p2.pinLedRed, HIGH);
      } else {
        Serial.println("vince P2");
        p2.pointsPlayer++;

        digitalWrite(p2.pinLedGreen, HIGH);
        //digitalWrite(p1.pinLedRed, HIGH);
      }
    }
    //CASO 2
    //vittoria diretta di P1 sui click
    if(p1.numClick == numClickRound && p2.numClick != numClickRound){
      Serial.println("vince P1");
      p1.pointsPlayer++;
      //penalita ad altro player
      p2.pointsPlayer--;
      digitalWrite(p1.pinLedGreen, HIGH);
      digitalWrite(p2.pinLedRed, HIGH);
    }
    //CASO 3
    //vittoria diretta di P2 sui punteggi
    if(p1.numClick != numClickRound && p2.numClick == numClickRound){
      Serial.println("vince P2");
      p2.pointsPlayer++;
      //penalita ad altro player
      p1.pointsPlayer--;
      digitalWrite(p2.pinLedGreen, HIGH);
      digitalWrite(p1.pinLedRed, HIGH);

    }
    //CASO 4
    //nessuno dei due ha fatto giusto
    if(p1.numClick != numClickRound && p2.numClick != numClickRound){
      Serial.println("vince nessuno");
      //penalita entrambi i player
      p2.pointsPlayer--;
      p1.pointsPlayer--;
      digitalWrite(p2.pinLedRed, HIGH);
      digitalWrite(p1.pinLedRed, HIGH);
    }
  }

  Serial.println("p1 click: " + String(p1.numClick));
  Serial.println("p2 click: " + String(p2.numClick));
  Serial.println("p1 points: " + String(p1.pointsPlayer));
  Serial.println("p2 points: " + String(p2.pointsPlayer));
  Serial.println(String(currentT1));
  Serial.println("-----------------");
  
  /*
  ho spostato tutti i delay e led off in un unico punto, 
  perche essendo a coppie accesi e spenti, 
  mi basta spegnerli tutti alla scadenza dei 2 secondi
  uso delay in quanto voglio usare una funzione bloccante per non andare aventi
  e mostrare correttamente ai giocatori lo status di gioco
  */

  //stampo su LCD i punteggi dei giocatori
  //a sinistra giocatore 1, a destra giocatore 2 
  lcd.setCursor(0, 0);
  lcd.print("P1: " + String(p1.pointsPlayer));
  lcd.setCursor(10, 0); 
  lcd.print("P2: " + String(p2.pointsPlayer));

  delay(2000);
  digitalWrite(p1.pinLedGreen, LOW);
  digitalWrite(p1.pinLedRed, LOW);
  digitalWrite(p2.pinLedGreen, LOW);
  digitalWrite(p2.pinLedRed, LOW);
  //cambio stato a wait for next game ==> NON PIU' , NON DEVE ESSERI ATTESA TRA I ROUND, IL GO! VA DRITTO AL TERMINE DI OGNI ROUND
  //gameStateActual = WAITING_FOR_NEXT_GAME ;  

  setupPlay(); //Prepara i dati del nuovo round!
  //cambio stato a PLAY
  gameStateActual = PLAY;
  //reset entrambi i click a zero
  p2.numClick=0;
  p1.numClick=0;
  //riabilito interrupt dopo averli disabilitati sopra
  attachInterrupt(digitalPinToInterrupt(p1.pinBtn), actionCounterPlayer1, RISING);
  attachInterrupt(digitalPinToInterrupt(p2.pinBtn), actionCounterPlayer2, RISING);

  /*
  riutilizzo roundStartTime e timeToWait per calcolare la 
  differenza di tempo per andare in chiusura gioco 
  o GAME_OVER nel metodo waitForNextGame
  */
  roundStartTime=millis(); 
}

/*
terminato un round i giocatori hanno 'currentT1' tempo per cliccare uno dei pulsanti.
il click farà ripartire un nuovo round --> quindi torniamo in stato di play
se non avviene alcun click si passa allo stato di GAME OVER, dove verranno forniti i 
risultati finali  
*/
/*
Il tempo di attesa è scaduto senza alcun click dei giocatori
*/ 
/*
void waitForNextGame(){
  
  if(millis() - roundStartTime >= timeToWait && (p1.numClick == 0 || p2.numClick == 0)) {
    lcd.clear(); // Pulisce lo schermo da errori o rimasugli
    gameStateActual = GAME_OVER;
    gameOverTimer= millis();
  }
  
  if(p1.numClick != 0 || p2.numClick != 0){
    //subito reset dei click
    p1.numClick = 0;
    p2.numClick = 0;
    setupPlay();
    gameStateActual = PLAY; 

  }
}

*/
/*
GAME OVER
La partita e' giunta al termine, quindi controllo  i punteggi e possibili pareggi e stampo su LCD l'esito.
*/
void gameOverFinish(){
  Serial.println("IN GAME OVER");
  if(millis() - gameOverTimer < TIME_BEFORE_SLEEPING){ 
      // riutilizzo TIME_BEFORE_SLEEPING = 10 sec per non usare magic numbers
      // pareggio
      if(p1.pointsPlayer == p2.pointsPlayer){
        showDisplay("Game Over - Tie");

      }else{
        //vince p1
        if(p1.pointsPlayer > p2.pointsPlayer){
          showDisplay("Game Over - The winner is P1", SCROLL_INTERVAL/2);

        }else{
          // caso vince p2
          // p1.pointsPlayer < p2.pointsPlayer
          showDisplay("Game Over - The winner is P2",SCROLL_INTERVAL/2);

        }
      }
      Serial.println(String(p1.pointsPlayer));
      Serial.println(String(p2.pointsPlayer));
  }else{
      // Sono passati 10 secondi. Eseguiamo il RESET TOTALE per la nuova partita
      currentT1 = MAX_WAIT;         // Ripristina il tempo di reazione iniziale
      isFirstRoundOfPlay = true;    // Permette di rileggere il potenziometro nel setupPlay
      gameStateActual = PREPARATION; // Torna alla schermata "Press Buttons to Start"
  }
}
