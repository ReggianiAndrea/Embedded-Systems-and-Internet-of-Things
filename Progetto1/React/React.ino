#include <LiquidCrystal_I2C.h>
#include <avr/sleep.h>

/* Wiring: SDA => A4, SCL => A5 */
/* I2C address of the LCD: 0x27 */
/* Number of columns: 20 rows: 4 */
#define LCD_COL 20
#define LCD_ROW 4
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,LCD_COL,LCD_ROW); 

#define PIN_RED_1 5
#define PIN_RED_2 6

#define PIN_GREEN_1 8
#define PIN_GREEN_2 9

#define BTN_PL_1 2
#define BTN_PL_2 3

#define POT A0

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


//variabile per lcd scroll
const int SCROLL_INTERVAL = 300;

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
  p1.lastClick= millis();
  p1.numClick++;
  //Serial.print("p1 click: " + String(p1.numClick));
}

void actionCounterPlayer2() {
  p2.lastClick= millis();
  p2.numClick++;
  //Serial.print("p2 click: " + String(p2.numClick));
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
  //currentMillis = millis();

  stateTimer = millis();
  Serial.println("preparazione");
}

/*
  per attesa giocatore.
  attesa di un interrupt che cambi lo stato di gameStateActual.
  al cambio di stato il ciclo while viene chiuso.
  e chiusa la funzione per ritornare al LOOP principale. 
*/
void readyToPlay() {
  
  const int MIN_BRIGHTNESS = 0;
  const int MAX_BRIGHTNESS = 255;
  //const int SCROLL_INTERVAL = 300;  //spostata a riga 40, mi serve anche in  altro
          // tempo di attesa per lo scorrimento di una nuova lettera
  const int TIME_BEFORE_SLEEPING = 10000;   // tempo che deve trascorrere prima di entrare in modalità sleep
  static int fadingValue1 = 15;             //quantità di luminosità da aggiungere o togliere al led 1
  static int fadingValue2 = 15;             //quantità di luminosità da aggiungere o togliere al led 2
  static int brightLevel1 = 0;              //quanto è luminoso il led 1
  static int brightLevel2 = 255;            //quanto è luminoso il led 2

  // Flag statico per verificare se è la prima esecuzione
  static bool isInitialized = false;

  //stateTimer = currentMillis;

  if (!isInitialized) {
    digitalWrite(p1.pinLedGreen, LOW);
    digitalWrite(p2.pinLedGreen, LOW);
    isInitialized = true;
  }

  ledAscending(PIN_RED_1, brightLevel1, fadingValue1, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  ledDescending(PIN_RED_2, brightLevel2, fadingValue2, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  Serial.println("fattiLED");
  showDisplay2(SCROLL_INTERVAL, "Welcome to the React! Game. Press Buttons to Start");
  Serial.println("fattoLCD");
  // controllo per verificare se è passato abbastanza tempo e si puo' entrare in modalità sleep
  if (currentMillis - stateTimer >= TIME_BEFORE_SLEEPING) {
        gameStateActual = SLEEP_MODE;
        Serial.println("FATTODORMIRRE");
    } 

  // controllo per verificare se btn1 e 2 sono stati premuti insieme e si puo' passare alla fase play
  // ATTENZIONE TROPPO VELOCE AGGIUNGERE RITARDO PER CLICKKKKKKKKKKKKK KKKKKKKKKKKKKKKKKKKKKKKKKKKKKK

  //DA CHIEDERE AL PROF SE I BOTTONI DEVONO ESERE PREMUTI SIMULTANEAMENTE O BASTA UNO DOPO ALTRO,Reggia propone in caso di usare 2 variabili per stato high e low
  if (digitalRead(p1.pinBtn) == HIGH || digitalRead(p2.pinBtn) == HIGH) {
    gameStateActual = PLAY;
    Serial.println("setupEn Play");
    setupPlay();
    //play();
  }
  Serial.println("readytoPlay");
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
  timeToWait = random(1000, 5001); // Da 1 a 5 secondi
  numClickRound =random(1, 4);    // Da 1 a 3 click
  
  roundStartTime = millis();       // Prende il tempo ESATTO di inizio
  isNumberSpawned = false;        // Flag per separare l'attesa dall'azione nella funzione succ.
  
  // Resetta i click qui (evita che contino eventuali click spammati durante l'attesa)
  p1.numClick = 0;
  p2.numClick = 0;
  lcd.setCursor(0, 0);
  lcd.println("GO!");
  lcd.clear();
  lcd.setCursor(0, 0);
  Serial.println("numero go");
}


// esegue il gioco vero e proprio
void play() {
  Serial.println("inPlay");
  const int MAX_WAIT = 5000; // tempo massimo che puo' passare prima di vedere il valore sul display
  const int MAX_NUM_CLICK = 3; // num massimo di click che puo' uscire casualmente
  
  /*  
  static int timeToWait = random(0, MAX_WAIT + 1); // tempo di attesa casuale prima che compaia il num.
  static int numClickRound = random (1, MAX_NUM_CLICK + 1); // num di click del round
  static int startTime = millis(); // salva il tempo iniziale per calcolare l'attesa prima di far uscire il num
  */
  // NUM MAGGGICOOOOOOOOOOOOOOOO
  /*  
  showDisplay(300, "Go!");
  */
  
  if(!isNumberSpawned) {
      //delay(5000);
      // Attesa prima che compaia il numero
      Serial.println("timetoplayNumSpownedddd");
      if(millis() - roundStartTime >= 2000) {//timeToWait
        // Il tempo di attesa è scaduto
        isNumberSpawned = true;
        lcd.clear(); // Pulisce lo schermo dal "Go!"
        Serial.println("inPlay e numSpown");
        timerForMatchInGame=millis();
      }
  } else {
    // Continua a far scorrere Go!
    //showDisplay(300, "Go!");
    //lcd.print("Go!");
    //showDisplay(SCROLL_INTERVAL, "GO!");
    
    // Il numero è uscito.
    // Qui andrà inserita la logica di countdown per dare ai giocatori un limite di tempo
    // per cliccare, e poi la transizione a CHECK_VICTORY
    Serial.println("else");
    showDisplay2(SCROLL_INTERVAL, String(numClickRound));//reggia : ho sostituito il magic number
  
    if(millis()-timerForMatchInGame > timeToWait){
      Serial.println("partita finita");
      //superato il tempo di gioco controllo lo stato dei punteggi
      // e salvo lultimo tempo di click di entrambi i giocatori --> nella funzione invocata al click
      // cosi da confrontare chi ha terminato prima
      // dopo nessa sequenza successiva controllo chi ha fatto il numero corretto di click 
      // quindi passo a check status victory ?
      gameStateActual=CHECK_VICTORY;
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
  Serial.println("dormendo");
  Serial.flush();
  delay(1000);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  

  sleep_enable();
  sleep_mode(); 
  // Il microcontrollore si ferma qui fino all'interrupt
  sleep_disable();
  Serial.println("svegliando");
  lcd.backlight();
  gameStateActual = PREPARATION;
}

void logicForCheckVictory(){
  //check se almeno una sequenza è corretta --> numero di click corretti
  // controllo clickNPlayer1 e clickNPlayer2, controllo chi ha fatto il numero di click corretti
  //se  entrambi hanno i click giusti andiamo in parità
  // se almeno unno dei due ha i criteri giusti entriamo in parità

  //dopo aver fatto le condizioni di vittoria per un giocatore 
  //aver applicato i malus al giocatore avversario, e attiviamo il led verde
  /* 
  le variabili
  volatile int pointsPlayer;   
  volatile int numClick;       
  int pinLedRed;
  int pinLedGreen;
  int pinBtn;
  unsigned long lastClick;
  */

  //caso pareggio click, controllo i tempi
  Serial.println("in check victory");
  if(p1.numClick == numClickRound && p2.numClick == numClickRound){
    if(p1.lastClick < p2.lastClick){
      Serial.println("vince P1");
      p1.pointsPlayer++;

      digitalWrite(p1.pinLedGreen, HIGH);
      delay(2000); // SENTIAMO CON IL PROOOOOOOOOOOOOFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
      digitalWrite(p1.pinLedGreen, LOW);
    } else {
      Serial.println("vince P2");
      p2.pointsPlayer++;

      digitalWrite(p2.pinLedGreen, HIGH);
      delay(2000);
      digitalWrite(p2.pinLedGreen, LOW);
    }
    Serial.println("Pl1 :" + String(p1.pointsPlayer) );
    Serial.println("Pl2 :" + String(p2.pointsPlayer));
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

    //penalita ad altro player
    p1.pointsPlayer--;
    digitalWrite(p2.pinLedRed, HIGH);
    digitalWrite(p1.pinLedRed, HIGH);
    delay(2000);
    digitalWrite(p2.pinLedRed, LOW);
    digitalWrite(p1.pinLedRed, LOW);
  }
    //cambio stato a wait for next game 
    gameStateActual = WAITING_FOR_NEXT_GAME ;
    p2.numClick=0;
    p1.numClick=0;
    //reset entrambi i click a zero

}


void setup() {
  // put your setup code here, to run once:
  gameStateActual = PREPARATION;

  // setup input e output player 1
  pinMode(p1.pinLedRed, OUTPUT);
  pinMode(p1.pinLedGreen, OUTPUT);
  pinMode(p1.pinBtn, INPUT);
  // BUGGGGGG AUMENTA ANCHE SENZA ESSERE IN PLAYYYYYYYYYYYYYYYYYYYY
  attachInterrupt(digitalPinToInterrupt(p1.pinBtn), actionCounterPlayer1, RISING);

  // setup input e output player 2
  pinMode(p2.pinLedRed, OUTPUT);
  pinMode(p2.pinLedGreen, OUTPUT);
  pinMode(p2.pinBtn, INPUT);
  attachInterrupt(digitalPinToInterrupt(p2.pinBtn), actionCounterPlayer2, RISING);

  lcd.init();
  lcd.backlight();

  Serial.begin(9600);
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
      //fading led red
      readyToPlay();
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
        Serial.println("CIPPA LIPPA");

      break;

    case GAME_OVER:

      break;
    default :
      gameStateActual = PREPARATION;
      Serial.println("pirlo c'é stato un bug");
  }
}
