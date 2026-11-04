#include "button_impl.h"
#include "Arduino.h"

ButtonImpl::ButtonImpl(int pin){
  this->pin = pin;
  this->statusBtn=false;
  pinMode(this->pin, INPUT);
} 



void ButtonImpl::isPressed(){
  if(digitalRead(this->pin)){
    this->statusBtn=!this->statusBtn;
    delay(150);

  }
}


bool ButtonImpl::getStatus(){
  return this->statusBtn;
}