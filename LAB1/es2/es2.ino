#define LED_PIN1 11
#define LED_PIN2 10
#define LED_PIN3 9
void setup() {
pinMode(LED_PIN1, OUTPUT);
pinMode(LED_PIN2, OUTPUT);
pinMode(LED_PIN3, OUTPUT);
Serial.begin(9600);
}

void loop() {
digitalWrite(LED_PIN1,HIGH);

delay(500);

digitalWrite(LED_PIN1,LOW);

delay(500);

digitalWrite(LED_PIN2,HIGH);

delay(500);

digitalWrite(LED_PIN2,LOW);

delay(500);

digitalWrite(LED_PIN3,HIGH);

delay(500);

digitalWrite(LED_PIN3,LOW);

delay(500);
}