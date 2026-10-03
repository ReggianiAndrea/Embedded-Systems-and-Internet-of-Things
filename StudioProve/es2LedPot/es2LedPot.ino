const int Led= 9;
const int Pot = A0 ; //Potenziometer only in Analog In side ov 5V


int sensorValue=0;
int output =0;

void setup(){
pinMode(Led,OUTPUT);
Serial.begin(9600);

}

void loop(){

sensorValue=analogRead(Pot);

output=map(sensorValue,0,1024,0,255);

analogWrite(Led,output);

Serial.print("sensor = " ); 
Serial.print(sensorValue);
Serial.print("\t output = ");
Serial.println(output);

delay(200);
}
