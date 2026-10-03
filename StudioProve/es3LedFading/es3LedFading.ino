const int led9 = 9;
int fadeAmount = 5;
int brightLevel =0;


void setup() {
  pinMode(led9,OUTPUT);
  Serial.begin(9600);
}

void loop() {
  analogWrite(led9,brightLevel);
  if(brightLevel==0 || brightLevel==255){
    fadeAmount=-fadeAmount;
  }
  brightLevel+=fadeAmount;
  delay(100);
}
