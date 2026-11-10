#include "../Button/ButtonImpl.h"
#include <Arduino.h>

ButtonImpl::ButtonImpl(int pin){
  this->pin = pin;
  pinMode(this->pin, INPUT);
} 

bool ButtonImpl::isPressed(){
    return digitalRead(this->pin)==LOW;
}
