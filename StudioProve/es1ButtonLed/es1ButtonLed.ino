const int Led12= 12;
const int Button2= 2 ;
int buttonState=0;

void setup(){
pinMode(Led12,OUTPUT);
pinMode(Button2,INPUT);

Serial.begin(9600);
}

void loop(){

  buttonState=digitalRead(Button2);

  if(buttonState==HIGH){
    digitalWrite(Led12,HIGH);
    delay(100);
  }else{
    digitalWrite(Led12,LOW);
    delay(100);
  }
}
