#define BUTTON_PIN 2

volatile int count;
int lastCountPrinted;
void setup() {
  // put your setup code here, to run once:
  pinMode(BUTTON_PIN,INPUT);
  count=0;
  lastCounted=0;
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
  count++;
}