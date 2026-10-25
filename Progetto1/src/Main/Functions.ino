//utilities

/*
funzione di reazione all'interrupt su BOTTONE 1
utilizza un controlo di debounce che permette di evitare che un singolo click venga contato molteplici volte
*/
void actionCounterPlayer1() {
  if (playInGame) {
    if(millis() - p1.lastClick > ISR_DEBOUNCE ){ 
      p1.numClick++;
      p1.lastClick = millis();
    }
  }
}

/*
funzione di reazione all'interrupt su BOTTONE 2
utilizza un controlo di debounce che permette di evitare che un singolo click venga contato molteplici volte
*/
void actionCounterPlayer2() {
  if (playInGame) {
    if(millis() - p2.lastClick > ISR_DEBOUNCE ){
      p2.numClick++;
      p2.lastClick = millis();
    }
  }
}

/*
funzione per Aumentare la luminosità del led
il valore massimo ammesso per brightness è 255, 
arrivato a tale valore il led dovrà assumere comportamento opposto
int pin --> pin del led
int &brightVal --> valore di luminosità attuale
int &fadingVal --> quanto aumentare o diminuire la luminosità
int minBright --> valore minimo ammesso
int maxBright --> valore massimo ammesso
*/
void ledAscending(int pin, int &brightVal, int &fadingVal) {
  analogWrite(pin, brightVal);
  brightVal += fadingVal;
  if(brightVal == MIN_BRIGHTNESS || brightVal == MAX_BRIGHTNESS){
    fadingVal =- fadingVal;
  }
}

/*
funzione per Aumentare la luminosità del led
il suo funzionamento è in direzione opposta
*/
void ledDescending(int pin, int &brightVal, int &fadingVal) {
  analogWrite(pin, brightVal);
  if(brightVal == MIN_BRIGHTNESS || brightVal == MAX_BRIGHTNESS){
    fadingVal =- fadingVal;
  }
  brightVal += fadingVal;
}

/*
funzione di lettura da potenziometro
mappa il valore letto con raw in un numero da 1 a 4 ,
che verrà utilizzato per determinare il factorF usato 
con 'potValue=updateDifficultyFromPot()'
per ridurre il tempo di gioco currentT1
*/
float updateDifficultyFromPot() { 

  int raw = analogRead(POT);
  float factorF=0.05;//set interno
  int difficultyLevel = map(raw, 0, 1023, 1, 4);/*per gestire le casistiche da potenziometro*/
  // imposta factorF per livello (più livello -> riduzione più forte)
  switch (difficultyLevel) {
    case 1:
      //factorF = 0.95; /*riduzione lieve*/
      factorF = 0.05;
      break;
    case 2:
      //factorF = 0.90; /*riduzione media*/
      factorF = 0.10;
      break;
    case 3:
      //factorF = 0.85; 
      factorF = 0.15;
      break;
    case 4:
      factorF = 0.20;
      //factorF = 0.80; /*molto difficile*/
      break;
    default:
      factorF = 0.10;
      //factorF = 0.9;
      break;
  }
  //Serial.println(String(factorF)+ "factorF");
  return factorF;
}

void showDisplay(String msg) {
  showDisplay(msg, SCROLL_INTERVAL);
}

// versione senza bug di cambio testo
void showDisplay(String msg, int scrollInterval) {
  // memorizza l'ultima stringa ricevuta per capire se il testo ricevuto in input 
  // è diverso o meno dal precedente
  static String lastMsg = "";
  // memorizza quando è stata aggiunta una lettera l'ultima volta
  static unsigned long lastAddCharTime = 0;
  // memorizza l'indice del primo carattere della sottostringa da mostrare 
  static int iFirstCharShow = 0;
  // se il testo è corto, quindi sta nel display una volta scritto non va toccato
  // finchè non cambia il testo in input, così il display non sfarfalla 
  static bool isPrinted = false;
  // variabile utile per creare frasi con spazi prima o dopo.
  // utile per scorrimento e frasi centrate
  static String paddedMsg = "";

  // se la frase passata è diversa resetto i co
  if(msg != lastMsg) {
    lastMsg = msg;
    lastAddCharTime = 0;
    iFirstCharShow = 0;
    isPrinted = false;

    // se il messaggio da mostrare è piu' grande del display aggiungo un numero di spazi pari alla grandezza
    // del display prima e dopo il messaggio.
    // Utile per effetto scorriemnto
    if(msg.length() > LCD_COL) {
      String spaces = "";
      for (int i = 0; i < LCD_COL; i++) {
        spaces += " ";
      }
      paddedMsg = spaces + msg + spaces;
    }
  }

  // se il testo è piu' grande del display 
  if(msg.length() > LCD_COL) {
    // verifica se è il necessario aggiornare il dispaly per mostrare una nuova lettera
    if (millis() - lastAddCharTime >= scrollInterval) {
      lastAddCharTime = millis();
      String displayMsg = paddedMsg.substring(iFirstCharShow, iFirstCharShow + LCD_COL);
      lcd.setCursor(0, 0);
      lcd.print(displayMsg);
      iFirstCharShow ++;

      // appena finisce di scrivere la frase si resetta
      if (iFirstCharShow + LCD_COL > paddedMsg.length()) {
        iFirstCharShow = 0;
      }
    }
  } else {
    // se il testo è piu' corto del display
    if(!isPrinted) {
      // variabile usata per calcolar quanti spazi devono essere aggiuni a sx per avere il testo centrato
      int nSpaces = ((LCD_COL - msg.length()) / 2 ) -1;
      String spaces = "";
      for (int i = 0; i < nSpaces; i++) {
        spaces += " ";
      }
      paddedMsg = spaces + msg;

      lcd.setCursor(0, 0);
      lcd.print(paddedMsg);
      isPrinted = true;
    }
  }
}

// imposta tutto il necessario per la fase play
void setupPlay() { 
  digitalWrite(p1.pinLedGreen, LOW);
  digitalWrite(p1.pinLedRed, LOW);

  digitalWrite(p2.pinLedGreen, LOW);
  digitalWrite(p2.pinLedRed, LOW);

  showDisplay("");
  lcd.clear();

  // Inizializza i parametri del round
  timeToWait = random(MIN_WAIT, MAX_WAIT+1); // Da 1 a 5 secondi
  numClickRound =random(1, 4);    // Da 1 a 3 click
  
  roundStartTime = millis();       // Prende il tempo ESATTO di inizio
  isNumberSpawned = false;        // Flag per separare l'attesa dall'azione nella funzione succ.
  
  // Resetta i click qui (evita che vengano contati eventuali click eseguiti durante l'attesa)
  p1.numClick = 0;
  p2.numClick = 0;
  lcd.setCursor(0, 0);
  //showDisplay("GO!");

}
