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
bool playInGame = false ; //variabile inizializzata in setupplay, usata come determinante per accettare o non gli interrupts
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

enum gameState {
  PREPARATION, //letture valori, counter restettati. perche il setup non puo essere ri-eseguito se arduino non viene riavviato
  READY_TO_PLAY, //potenziometro settato, in attesa di avviare la partita
  SLEEP_MODE, //entriamo in questa fase se nella READY_TO_PLAY i pulsanti non vengono cliccati entrambi entro 10 s
  PLAY, //stato di gioco
  CHECK_VICTORY, // verifica dei click dei giocatori e dei loro tempi di reazione
  //WAITING_FOR_NEXT_GAME, //attesa tra un round e il successivo
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
  playInGame = true;
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
/*
    case WAITING_FOR_NEXT_GAME:
      waitForNextGame();
      break;
*/
    case GAME_OVER:
      gameOverFinish();
      break;
    default :
      gameStateActual = PREPARATION;
  }
}
