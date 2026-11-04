/*
* Example/test using analog temperature sensor LM35
*
*
*/
// #include <LiquidCrystal_I2C.h> 

// LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,20,4); 

void setup() {
  // lcd.init();
  // lcd.backlight();
  Serial.begin(9600);
}

void loop() {
  int value = analogRead(A0);
  
  /* value : 1024 = value_in_mV : 5000 */
  /* value_in_mV = 5000/1024 * value = 4.883 * value */
  
  int value_in_mV = 4.883 * value; 

/* 
   * Linear characteristics for sensors:
   * - Y(X) = K*X + Y0
   * For LM35 - Y(X) = Voltage V, X = Temperature T
   * - V(T) = K*T + V0 = 0.01 * T 
   *   From the datasheet: 
   *   "OUTPUT = 0 mV + 10.0 mV/C", that is: V = 0.01*T
   * - Reversing the formula: T(V) = V/0.01
   */
 
  double value_in_C = value_in_mV / 0.01;
  Serial.println(value_in_C);
  
  /*
  lcd.setCursor(0, 10); // Set the cursor on the third column and first row.
  lcd.print(String("Value in C°: ") + value_in_C);
  */
  delay(1000);
}