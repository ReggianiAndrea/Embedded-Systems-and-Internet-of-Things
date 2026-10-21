#include <LiquidCrystal_I2C.h>
#include <avr/sleep.h>

#define PIN_RED_1 5
#define PIN_RED_2 6

#define PIN_GREEN_1 8
#define PIN_GREEN_2 9

#define BTN_PL_1 2
#define BTN_PL_2 3

#define POT A0

#define ISR_DEBOUNCE 280   /*tempo di debounce in ms*/
/* Wiring: SDA => A4, SCL => A5 */
/* I2C address of the LCD: 0x27 */
/* Number of columns: 20 rows: 4 */
#define LCD_COL 20
#define LCD_ROW 4
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,LCD_COL,LCD_ROW); 


const int MIN_BRIGHTNESS = 0;
const int MAX_BRIGHTNESS = 255;
 
const int TIME_BEFORE_SLEEPING = 10000;   // tempo che deve trascorrere prima di entrare in modalità sleep
  
const int SCROLL_INTERVAL = 300; //variabile per lcd scroll + tempo di attesa per lo scorrimento di una nuova lettera
const int MAX_WAIT = 5000; // tempo massimo che puo' passare prima di vedere il valore sul display
const int MIN_WAIT = 1000; // tempo minimo che puo' passare prima di vedere il valore sul display
const int MAX_NUM_CLICK = 3; // num massimo di click che puo' uscire casualmente


/* 
Flag per verificare se è la prima esecuzione 
viene impostato a true nel MAIN setup
*/
bool isFirstRoundOfPlay = false; 


struct Player {
  volatile int pointsPlayer;   // punti totali del giocatore
  volatile int numClick;       // num di click del giocatore nel round
  int pinLedRed;
  int pinLedGreen;
  int pinBtn;
  unsigned long lastClick;
};
Player p1 = {0, 0, PIN_RED_1, PIN_GREEN_1, BTN_PL_1};
Player p2 = {0, 0, PIN_RED_2, PIN_GREEN_2, BTN_PL_2};

// Variabili di stato per il round
unsigned long roundStartTime = 0;
int timeToWait = 0;
int numClickRound = 0; //variabile per generare e salvare il numero casuale da 1 a 3
bool isNumberSpawned = false; //variabile flag per determinare generazione del numero 

/*variabili per timers*/
unsigned long currentMillis;
unsigned long stateTimer;
unsigned long timerForMatchInGame;

enum gameState {
  PREPARATION, //letture valori, counter restettati. perche il setup non puo essere ri-eseguito se arduino non viene riavviato
  READY_TO_PLAY, //potenziometro settato, in attesa di avviare la partita
  SLEEP_MODE,
  PLAY,
  CHECK_VICTORY,
  WAITING_FOR_NEXT_GAME,
  GAME_OVER
};
enum gameState gameStateActual;

//utilities

void actionCounterPlayer1() {
  if(millis() - p1.lastClick > ISR_DEBOUNCE ){ 
    p1.numClick++;
    //Serial.println(String(p1.numClick));
    p1.lastClick = millis();
  }
}

void actionCounterPlayer2() {
  if(millis() - p2.lastClick > ISR_DEBOUNCE ){
    p2.numClick++;
    //Serial.println(String(p1.numClick));
    p2.lastClick = millis();
  }
}


void ledAscending(int pin, int &brightVal, int &fadingVal, int minBright, int maxBright) {
  analogWrite(pin, brightVal);
  brightVal += fadingVal;
  if(brightVal == minBright || brightVal == maxBright){
    fadingVal =- fadingVal;
  }
}

void ledDescending(int pin, int &brightVal, int &fadingVal, int minBright, int maxBright) {
  analogWrite(pin, brightVal);
  if(brightVal == minBright || brightVal == maxBright){
    fadingVal =- fadingVal;
  }
  brightVal += fadingVal;
}

float updateDifficultyFromPot() { 
//chiamata solo nel setupdi ogni prima Partita
//non round
  int raw = analogRead(POT);
  //float 
  float factorF=0.90;
  int difficultyLevel = map(raw, 0, 1023, 1, 4);/*per gestire le casistiche da potenziometro*/
  // imposta factorF per livello (più livello -> riduzione più forte)
  switch (difficultyLevel) {
    case 1:
      factorF = 0.95; /*riduzione lieve*/
    case 2:
      factorF = 0.90; /*riduzione media*/
    case 3:
      factorF = 0.85; 
    case 4:
      factorF = 0.80; /*molto difficile*/
    default:
      factorF = 0.90;
  }
  Serial.println(String(factorF)+ "factorF");
  return factorF;
}

// metodi veri

/*
  fase di preparazione del gioco, in caso di sconfitta
  Se l'arduino uno non viene resettato il setup non viene rilanciato,
  risulta quindi necessaria una ulteriore fase di preparazione
*/
void preparationGame(){
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
  //Serial.println("preparazione");
}

/*
  per attesa giocatore.
  attesa di un interrupt che cambi lo stato di gameStateActual.
  al cambio di stato il ciclo while viene chiuso.
  e chiusa la funzione per ritornare al LOOP principale. 
*/
void readyToPlay() {
  static int fadingValue1 = 15;             //quantità di luminosità da aggiungere o togliere al led 1
  static int fadingValue2 = 15;             //quantità di luminosità da aggiungere o togliere al led 2
  static int brightLevel1 = 0;              //quanto è luminoso il led 1
  static int brightLevel2 = 255;            //quanto è luminoso il led 2

  if (isFirstRoundOfPlay) {
    digitalWrite(p1.pinLedGreen, LOW);
    digitalWrite(p2.pinLedGreen, LOW);
    isFirstRoundOfPlay = false;
  }

  ledAscending(PIN_RED_1, brightLevel1, fadingValue1, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  ledDescending(PIN_RED_2, brightLevel2, fadingValue2, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  //Serial.println("fattiLED");
  showDisplay2(SCROLL_INTERVAL, "Welcome to the React! Game. Press Buttons to Start");
  //Serial.println("fattoLCD");
  // controllo per verificare se è passato abbastanza tempo e si puo' entrare in modalità sleep
  if (currentMillis - stateTimer >= TIME_BEFORE_SLEEPING) {
        gameStateActual = SLEEP_MODE;
        //Serial.println("FATTODORMIRRE");
    } 

  //DA CHIEDERE AL PROF SE I BOTTONI DEVONO ESERE PREMUTI SIMULTANEAMENTE O BASTA UNO DOPO ALTRO,Reggia propone in caso di usare 2 variabili per stato high e low
  if (digitalRead(p1.pinBtn) == HIGH || digitalRead(p2.pinBtn) == HIGH) {
    /*
    cambio lo stato e passo al play();
    
    */
    gameStateActual = PLAY;

    //Serial.println("setupEn Play");
    setupPlay();
  }
  //Serial.println("readytoPlay");
}

// Mostra una frase a scorrimento su una sola riga ed è indipendente dal tipo di display.
void showDisplay(int scrollInterval, String msg) {
  // messaggio con aggiunti gli spazi della larghezza del display, utili per effetto scorriemnto
  String paddedMsg = "";
  // stringa che contiene gli spazi della larghezza del display per l'effetto scorrimento
  String spaces = "";

  // memorizza quando è stata aggiunta una lettera l'ultima volta
  static unsigned long lastAddCharTime = 0;
  // memorizza l'indice del primo carattere della sottostringa da mostrare 
  static int iFirstCharShow = 0;
  
  for (int i = 0; i < LCD_COL; i++) {
    spaces += " ";
  }
  paddedMsg = spaces + msg + spaces;

  // verifica se è il necessario aggiornare il dispaly per mostrare una nuova lettera
  if (millis() - lastAddCharTime >= scrollInterval) {
    lastAddCharTime = millis();
    String displayMsg = paddedMsg.substring(iFirstCharShow, iFirstCharShow + LCD_COL);
    lcd.setCursor(0, 0);
    lcd.print(displayMsg);
    iFirstCharShow ++;

    // appena finisce di scrivere la frase si resetta
    // mettere > o >= ??????????
    if (iFirstCharShow + LCD_COL > paddedMsg.length()) {
      iFirstCharShow = 0;
    }
  }
}

// versione senza bug di cambio testo
void showDisplay2(int scrollInterval , String msg) {
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
  timeToWait = random(MIN_WAIT, MAX_WAIT+1) * updateDifficultyFromPot(); // Da 1 a 5 secondi
  numClickRound =random(1, 4);    // Da 1 a 3 click
  
  roundStartTime = millis();       // Prende il tempo ESATTO di inizio
  isNumberSpawned = false;        // Flag per separare l'attesa dall'azione nella funzione succ.
  
  // Resetta i click qui (evita che contino eventuali click spammati durante l'attesa)
  p1.numClick = 0;
  p2.numClick = 0;
  lcd.setCursor(0, 0);
  //lcd.println("GO!");
  showDisplay2(SCROLL_INTERVAL, "GO!");
/*  
  delay(200);
  lcd.clear();
  lcd.setCursor(0, 0);*/
  //Serial.println("numero go");
}


// esegue il gioco vero e proprio
void play() {
  //Serial.println("inPlay");
  if(!isNumberSpawned) {
      // Attesa prima che compaia il numero
      //Serial.println("timetoplayNumSpownedddd");
      if(millis() - roundStartTime >= timeToWait) {
        // Il tempo di attesa è scaduto
        isNumberSpawned = true;
        lcd.clear(); // Pulisce lo schermo dal "Go!"
        //Serial.println("inPlay e numSpown");
        timerForMatchInGame=millis();
        /*
        TODOOOOOOOO metodo resetclick e penalita
        se un interrupt viene eseguito qui, 
        malus al giocatore corrispondente, 
        led rossi immediatamente  e -1 punto
        */
      }
  } else {
    
    // Il numero è uscito.
    // Qui andrà inserita la logica di countdown per dare ai giocatori un limite di tempo
    // per cliccare, e poi la transizione a CHECK_VICTORY
    //Serial.println("else");

    showDisplay2(SCROLL_INTERVAL, String(numClickRound));
    //reggia : ho sostituito il magic number
  
    if(millis()-timerForMatchInGame > timeToWait){
      //Serial.println("partita finita");
      //superato il tempo di gioco controllo lo stato dei punteggi
      // e salvo lultimo tempo di click di entrambi i giocatori --> nella funzione invocata al click
      // cosi da confrontare chi ha terminato prima
      // dopo nessa sequenza successiva controllo chi ha fatto il numero corretto di click 
      // quindi passo a check status victory ?
              
        /*
        TODOOOOOOOO  metodo resetclick e penalita
        se un interrupt viene eseguito qui, 
        malus al giocatore corrispondente, 
        led rossi immediatamente  e -1 punto
        */


        /*
              gameStateActual=CHECK_VICTORY;
      G1 3
       milluis 3 in tempo
      G2 3 4 con timer gia scaduto ++> -1 click penalita,
        millis associato a 4 click , ma 4 click fuori

        */
    }
  }
} 


void deepSleepWhileWait(){
  lcd.clear();
  lcd.noBacklight();
  analogWrite(PIN_RED_1, 0);
  analogWrite(PIN_RED_2, 0);
  digitalWrite(PIN_GREEN_1, LOW);
  digitalWrite(PIN_GREEN_2, LOW);
  //Serial.println("dormendo");
  Serial.flush();
  delay(MIN_WAIT); //uso MIN_WAIT per non usare magic numbers  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  

  sleep_enable();
  sleep_mode(); 
  // Il microcontrollore si ferma qui fino all'interrupt
  sleep_disable();
  //Serial.println("svegliando");
  lcd.backlight();
  gameStateActual = PREPARATION;
}

void logicForCheckVictory(){
  //check se almeno una sequenza è corretta --> numero di click corretti
  // controllo clickNPlayer1 e clickNPlayer2, controllo chi ha fatto il numero di click corretti
  //se entrambi hanno i click giusti andiamo in parità --> guardiamo i tempi, 
  //caso tempi ugualui lo gestiamo ???????????????
  // se almeno unno dei due ha i criteri giusti entriamo in parità

  //dopo aver fatto le condizioni di vittoria per un giocatore 
  //aver applicato i malus al giocatore avversario, e attiviamo il led verde


  /*
    disabilito iterrupt per non permettere ai giocatori 
    di cliccare dopo la scadenza del timer
  */
  detachInterrupt(digitalPinToInterrupt(p1.pinBtn));
  detachInterrupt(digitalPinToInterrupt(p2.pinBtn));

  //caso pareggio click, controllo i tempi
  //Serial.println("in check victory");
  if(p1.numClick == numClickRound && p2.numClick == numClickRound){
    if(p1.lastClick < p2.lastClick){
      Serial.println("vince P1");
      p1.pointsPlayer++;

      digitalWrite(p1.pinLedGreen, HIGH);
      delay(2000); // SENTIAMO CON IL PROOOOOOOOOOOOOFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
      //LO CONSIDERIAMO COME UNA FASE DI STOP OPPURE UTILIZZIAMO MILLIS() CHIADIAMO PER SICUREZZA
      digitalWrite(p1.pinLedGreen, LOW);
    } else {
      Serial.println("vince P2");
      p2.pointsPlayer++;

      digitalWrite(p2.pinLedGreen, HIGH);
      delay(2000);
      digitalWrite(p2.pinLedGreen, LOW);
    }
  }

  //vittoria diretta di P1 sui click
  if(p1.numClick == numClickRound && p2.numClick != numClickRound){
    Serial.println("vince P1");
    p1.pointsPlayer++;

    //penalita ad altro player
    p2.pointsPlayer--;
    digitalWrite(p1.pinLedGreen, HIGH);
    delay(2000);
    digitalWrite(p1.pinLedGreen, LOW);
  }

  //vittoria diretta di P2 sui punteggi
  if(p1.numClick != numClickRound && p2.numClick == numClickRound){
    Serial.println("vince P2");
    p2.pointsPlayer++;

    //penalita ad altro player
    p1.pointsPlayer--;
    digitalWrite(p2.pinLedGreen, HIGH);
    delay(2000);
    digitalWrite(p2.pinLedGreen, LOW);

  }

  //nessuno dei due ha fatto giusto
  if(p1.numClick != numClickRound && p2.numClick != numClickRound){
    Serial.println("vince nessuno");
    p2.pointsPlayer--;

    //penalita entrambi i player
    p1.pointsPlayer--;
    digitalWrite(p2.pinLedRed, HIGH);
    digitalWrite(p1.pinLedRed, HIGH);
    delay(2000);
    digitalWrite(p2.pinLedRed, LOW);
    digitalWrite(p1.pinLedRed, LOW);
  }
   

   
  Serial.println("p1 click: " + String(p1.numClick));
  Serial.println("p2 click: " + String(p2.numClick));
  delay(2000);

  //cambio stato a wait for next game
  gameStateActual = WAITING_FOR_NEXT_GAME ;

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


void waitForNextGame(){
  /*
    terminato un round i giocatori hanno T1 tempo per cliccare uno dei pulsanti.
    il click fara' ripartire un nuovo round --> quindi torniamo in stato di play
    nessun click fara' andare in stato di GAME OVER, dove verranno forniti i 
    risultati finali  
    se nessun bottone e' stato cliccato chiudo il gioco
  */
  if(millis() - roundStartTime >= timeToWait && (p1.numClick == 0 || p2.numClick == 0)) {
    // Il tempo di attesa è scaduto senza alcun clickdei giocatori
    lcd.clear(); // Pulisce lo schermo da errori o rimasugli
    gameStateActual = GAME_OVER;
  }
  
  /*
    i giocatori cliccano, quindi vogliono rigiocare,continuare i round
    quindi devo ricalcolare T1 che scenda, non deve essere solo randomico e 
    inoltre deve essere ricalcolato in NUMERO da 1 a 3
    quindi andrebbe rieseguito setupPlay, ma setUpPlay contiene solo lo stretto necessario ? 
    nel senso contiene solo  
  */
  if(p1.numClick != 0 || p2.numClick != 0){
    //subito reset dei click
    p1.numClick = 0;
    p2.numClick = 0;
    gameStateActual = PLAY; 

  }
}

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

  /*
  Setting dei parametri per lettura accurata di 
  pressione dei pulsanti senza bouncing, 
  modo migliore per non dover fare uso di polling.
  Parametri associati alle struct dei Players
  */

  
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
  // ricordiamoci il potenziometro per aumentare la difficoltà
  
  
  //fare gestione del bouncing
  currentMillis = millis();
  switch(gameStateActual){
    case PREPARATION :
      //read del potenziometro
      preparationGame();
      break;
    case READY_TO_PLAY: 
      //fading led red
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
    
        //Serial.println("WAITING for next GAME");
      break;

    case GAME_OVER:
      //stampo i risultati e chiudo ?10 secondi e si riparte con WELCOME TO JUMANJI 
      Serial.println("GAME OVER");
        //riparte dal play, quindi devo rifare il setup 
      break;
    default :
      gameStateActual = PREPARATION;
      //Serial.println(" c'é stato un bug");
  }
  
}
