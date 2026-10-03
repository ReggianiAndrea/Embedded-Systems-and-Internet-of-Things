const int LED=13;
const int BUTTON=9;

int buttonState=0;
 bool ledAlreadyOn = false;

void setup() {
pinMode(LED,OUTPUT);
pinMode(BUTTON,INPUT);

Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  buttonState=digitalRead(BUTTON);

  if(buttonState==HIGH && !ledAlreadyOn){
    digitalWrite(LED,HIGH);
    Serial.println("Button pressed, LED ON.");
    ledAlreadyOn=true;
  }else if(buttonState==LOW && ledAlreadyOn){
    delay(20);
    digitalWrite(LED,LOW);
    ledAlreadyOn=false;
  }
}
