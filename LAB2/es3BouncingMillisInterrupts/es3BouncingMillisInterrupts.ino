#define BUTTON_PIN 2
#define DEBOUNCE_TIME 20

volatile int count;
int lastCountPrinted;
unsigned long previusPressedTime;

void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON_PIN,INPUT);
  count=0;
  lastCounted=0;
  previusPressedTime=0;
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),inc,RISING);
  Serial.begin(9600);
}

void loop() {
  noInterrupts();
  int currentCount=count;
  interrupts();

  if(count != lastCountPrinted){
    Serial.println(String("count:")+currentCount);
    lastCountPrinted= currentCount;
    delay(1000);
  }
}


void inc(){
  unsigned long currentTime= millis();
  if(currentTIme-previusPressedTIme > DEBOUNCE_TIME){
    count++;
    previusPressedTIme = currentTime;
  }
}