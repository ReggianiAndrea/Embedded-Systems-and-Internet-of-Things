#include <LiquidCrystal_I2C.h>
#include <avr/sleep.h>

#define PIN_RED_1 5
#define PIN_RED_2 6

#define PIN_GREEN_1 8
#define PIN_GREEN_2 9

#define BTN_PL_1 2
#define BTN_PL_2 3

#define POT A0

#define ISR_DEBOUNCE 280   /*tempo di debounce in ms, un valore inferiore potrebbe non funzionare e leggere più click al posto di 1*/
/* Wiring: SDA => A4, SCL => A5 */
/* I2C address of the LCD: 0x27 */
/* Number of columns: 20 rows: 4 */
#define LCD_COL 20
#define LCD_ROW 4
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,LCD_COL,LCD_ROW); 

const int MIN_BRIGHTNESS = 0;
const int MAX_BRIGHTNESS = 255;
 
const int TIME_BEFORE_SLEEPING = 10000;   // tempo che deve trascorrere prima di entrare in SLEEP_MODE
  
const int SCROLL_INTERVAL = 300; //variabile per lcd scroll
const int MAX_WAIT = 5000; // tempo massimo che puo' passare prima di vedere il valore sul display
const int MIN_WAIT = 1000; // tempo minimo che puo' passare prima di vedere il valore sul display
const int MAX_NUM_CLICK = 3; // num massimo di click che puo' uscire casualmente, che i giocatori devono fare

double currentT1 = MAX_WAIT;  // Tempo di reazione che si accorcerà, tempo iniziale

// Flag per verificare se è la prima esecuzione, viene impostato a true nel setUp
bool isFirstRoundOfPlay = false;

struct Player {
  volatile int pointsPlayer;   // punti totali del giocatore
  volatile int numClick;       // num di click del giocatore nel round
  int pinLedRed;
  int pinLedGreen;
  int pinBtn;
  unsigned long lastClick;   //variabile usate per determinare il tempo dell'ultimo click
};
Player p1 = {0, 0, PIN_RED_1, PIN_GREEN_1, BTN_PL_1};
Player p2 = {0, 0, PIN_RED_2, PIN_GREEN_2, BTN_PL_2};

// Variabili di stato per il round
unsigned long roundStartTime = 0;
int timeToWait = 0;   //tempo che verrà inizializzato randomicamente ad ogni round per mostrare il numero sul display
int numClickRound = 0; //variabile per salvare il numero casuale da 1 a 3
bool isNumberSpawned = false; //variabile flag per determinare se è stato generato il numero 
double potValue = 0.0; //variabile per lettura potenziometro all avvio di ogni nuova partita; la si moltiplica con currentT1; non viene ricalcolata ad ogni round

/*variabili per timers*/
unsigned long currentMillis; //variabile da confrontare con millis() per verificare lo scorrere del tempo
unsigned long stateTimer; //variabile utile per determinare se è il momento di andare in SLEEP_MODE
unsigned long timerForMatchInGame; //variabile usata per determinare il momento di termine del round, quando millis()-timerForMatchInGame > tempo T1, il round termina
unsigned long gameOverTimer; //variabile per countdown nell stato di GameOver, 10 secondi dopo essere entrati nello stato di GAME_OVER, il gioco ripartirà dal 'WELCOME....' 

//utilities

/*
funzione di reazione all'interrupt su BOTTONE 1
utilizza un controlo di debounce che permette di evitare che un singolo click venga contato molteplici volte
*/
void actionCounterPlayer1() {
  if(millis() - p1.lastClick > ISR_DEBOUNCE ){ 
    p1.numClick++;
    p1.lastClick = millis();
  }
}

/*
funzione di reazione all'interrupt su BOTTONE 2
utilizza un controlo di debounce che permette di evitare che un singolo click venga contato molteplici volte
*/
void actionCounterPlayer2() {
  if(millis() - p2.lastClick > ISR_DEBOUNCE ){
    p2.numClick++;
    p2.lastClick = millis();
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
  showDisplay("GO!");

}

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


enum gameState {
  PREPARATION, //letture valori, counter restettati. perche il setup non puo essere ri-eseguito se arduino non viene riavviato
  READY_TO_PLAY, //potenziometro settato, in attesa di avviare la partita
  SLEEP_MODE, //entriamo in questa fase se nella READY_TO_PLAY i pulsanti non vengono cliccati entrambi entro 10 s
  PLAY, //stato di gioco
  CHECK_VICTORY, // verifica dei click dei giocatori e dei loro tempi di reazione
  WAITING_FOR_NEXT_GAME, //attesa tra un round e il successivo
  GAME_OVER // se nello stato di WAITING_FOR_NEXT_GAME, non avvengono click, mostriamo i punteggi dei giocatori e decretiamo il vincitore della partita; dopo si riparte da capo.
};
enum gameState gameStateActual;

void setup() {
  gameStateActual = PREPARATION;

  // setup input e output player 1
  pinMode(p1.pinLedRed, OUTPUT);
  pinMode(p1.pinLedGreen, OUTPUT);
  pinMode(p1.pinBtn, INPUT);
  attachInterrupt(digitalPinToInterrupt(p1.pinBtn), actionCounterPlayer1, RISING);

  // setup input e output player 2
  pinMode(p2.pinLedRed, OUTPUT);
  pinMode(p2.pinLedGreen, OUTPUT);
  pinMode(p2.pinBtn, INPUT);
  attachInterrupt(digitalPinToInterrupt(p2.pinBtn), actionCounterPlayer2, RISING);

  lcd.init();
  lcd.backlight();
  isFirstRoundOfPlay = true;
  //currentT1 = MAX_WAIT;  // Tempo di reazione che si accorcerà, tempo iniziale

  Serial.begin(9600);

  /*
  serve per la inizializzazione
  corretta del seme di random, 
  altrimenti arduino utilizza 
  sempre la stessa tipologia
  */
  randomSeed(analogRead(A1));

  /*
  una chiamata per non duplicare il codice di impostazione dei parametri
  */
  preparationGame();
}

void loop() {
  currentMillis = millis();
  switch(gameStateActual){
    case PREPARATION :
      preparationGame();
      break;
    case READY_TO_PLAY:
      readyToPlay();
      /*
      con la read calcolo il prodotto del valore
      del potenziometro x T1 ,
      in modo da ridurre il tempo, 
      valore deve essere < 1
      */
      break;
    case SLEEP_MODE:
      deepSleepWhileWait();
      break;
    case PLAY:
      play();
      break;
    case CHECK_VICTORY:
      logicForCheckVictory();
      break;
    case WAITING_FOR_NEXT_GAME:
      waitForNextGame();
      break;
    case GAME_OVER:
      gameOverFinish();
      break;
    default :
      gameStateActual = PREPARATION;
  }
}
