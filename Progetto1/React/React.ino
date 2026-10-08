#include <LiquidCrystal_I2C.h> 

/* Wiring: SDA => A4, SCL => A5 */
/* I2C address of the LCD: 0x27 */
/* Number of columns: 20 rows: 4 */
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,20,4); 

#define PIN_RED_1 5
#define PIN_RED_2 6

#define PIN_GREEN_1 8
#define PIN_GREEN_2 9

#define BTN_PL1 1
#define BTN_PL2 2

#define POT A0

int fadingValue1 = 15 ; //quantità di luminosita da aggiungere o togliere al led 1
int fadingValue2 = 15 ; //quantità di luminosita da aggiungere o togliere al led 2
int brightLevel1 = 0; //quanto e luminoso il led 1
int brightLevel2 =255; //quanto e luminoso il led 2

volatile int pointsPlayer1; //punti al giocatore 1
volatile int pointsPlayer2; //punti al giocatore 2
volatile int clickNPlayer1; //click del giocatore 1
volatile int clickNPlayer2; //click del giocatore 2

unsigned long currentMillis;

enum gameState {
  PREPARATION, //letture valori, counter restettati. perche il setup non puo essere ri-eseguito se arduino non viene riavviato
  READY_TO_PLAY, //potenziometro settato, in attesa di avviare la partita
  SLEEP_MODE,
  CALCULATE_POINTS,
  PLAY,
  CHECK_VICTORY,
  POINT_TO_PLAYER,
};
enum gameState gameStateActual ;


/*
  fase di preparazione del gioco, in caso di sconfitta
  Se l'arduino uno non viene resettato il setup non viene rilanciato,
  risulta quindi necessaria una ulteriore fase di preparazione
*/
void preparationGame(){
  pointsPlayer1=0;
  pointsPlayer2=0;
  clickNPlayer1=0;
  clickNPlayer2=0;

  //Serial.println("preparationGame");
  //delay(1000);
  gameStateActual = READY_TO_PLAY;
}

/*
  per attesa giocatore.
  attesa di un interrupt che cambi lo stato di gameStateActual.
  al cambio di stato il ciclo while viene chiuso.
  e chiusa la funzione per ritornare al LOOP principale. 
*/
void readyToPlay(){
  digitalWrite(PIN_GREEN_1, LOW);
  digitalWrite(PIN_GREEN_2, LOW);
  currentMillis = millis();
  while(gameStateActual==READY_TO_PLAY){
    analogWrite(PIN_RED_1,brightLevel1);
    analogWrite(PIN_RED_2,brightLevel2);
    brightLevel1+=fadingValue1;
    if(brightLevel1==0 || brightLevel1==255){
      fadingValue1=-fadingValue1;
    }
    //delay(100);
    if(brightLevel2==0 || brightLevel2==255){
      fadingValue2=-fadingValue2;
    }
    brightLevel2+=fadingValue2;
    delay(500);
    Serial.println(brightLevel1);
    delay(200);
    Serial.println(brightLevel2);
    delay(200);
    //Serial.println("readyToPlay");
    /* funziona ma il prof vuole tutto su una sola riga a scorrimento, quindi da trovare altro approccio
    lcd.clear();
    lcd.setCursor(0,0); //colonna e riga
    lcd.print("Welcome to the ");
    delay(800);
    lcd.setCursor(0,1);
    lcd.println("React! Game.");
    delay(800);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.println("Press Buttons");
    delay(800);
    lcd.setCursor(0,1);
    lcd.println("to Start!");
    delay(1000);
    */    
  }
}

void actionCounterPlayer1() {
  clickNPlayer1++;
}

void actionCounterPlayer2() {
  clickNPlayer2++;
}

void setup() {
  // put your setup code here, to run once:
  gameStateActual = PREPARATION;

  pinMode(PIN_GREEN_1,OUTPUT);
  pinMode(PIN_GREEN_2,OUTPUT);

  pinMode(PIN_RED_1,OUTPUT);
  pinMode(PIN_RED_2,OUTPUT);

  pinMode(BTN_PL1,INPUT);
  pinMode(BTN_PL2,INPUT);

  attachInterrupt(digitalPinToInterrupt(BTN_PL1), actionCounterPlayer1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_PL2), actionCounterPlayer2, FALLING);

  lcd.init();
  lcd.backlight();

  Serial.begin(9600);
  /*
  una chiamata per non duplicare il codice di impostazione dei parametri
  */
  preparationGame();
}

void loop() {
  // put your main code here, to run repeatedly:
  switch(gameStateActual){
    case PREPARATION :
      preparationGame();
      break;
    case READY_TO_PLAY: 
      //fading led red
      readyToPlay();
      //manca il calcolo con millis per mandare a dormire
      break;
    case SLEEP_MODE:
    case PLAY:
    case CALCULATE_POINTS:
    case CHECK_VICTORY:
    case POINT_TO_PLAYER:
    default :
      gameStateActual = PREPARATION;
      Serial.println("pirlo c'é stato un bug");
  }
}
