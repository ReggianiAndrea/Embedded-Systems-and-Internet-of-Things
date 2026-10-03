#define LED_PIN1 11
#define LED_PIN2 10
#define LED_PIN3 9

int value = 0;
int increment = 1;
int myPins[3] = {LED_PIN1, LED_PIN2, LED_PIN3};

void setup() {
  // put your setup code here, to run once:
  for (int i = 0; i < 3; i++) {
    pinMode(myPins[i], OUTPUT);
  }
}


void loop() {
  // put your main code here, to run repeatedly:

  if(value>=3 || value <0){
    increment = -increment;
  }
  digitalWrite(myPins[value],HIGH);
  delay(300);

  digitalWrite(myPins[value],LOW);

  value=value+increment;
  delay(300);



}
