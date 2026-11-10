#include <Arduino.h>
#include "LightImpl.h"

LightImpl::LightImpl(int pin){
    this->pin=pin;
    pinMode(this->pin,OUTPUT);
}

void LightImpl::switchOn(){ 
    digitalWrite(pin, HIGH); 
}

void LightImpl::switchOff(){ 
    digitalWrite(pin, LOW); 
}

bool LightImpl::getStatus(){
    return this->status;
}
