#include <Arduino.h>
#include "LightTask.h"
#include "../Light/LightImpl.h"

LightTask::LightTask(int pin,bool*msg){
    this->pin=pin;
    this->msg=msg;
}

void LightTask::init(int period){
    Task::init(period);
    light=new LightImpl(this->pin);
    this->lightState=OFF;
}

void LightTask::tick(){
    switch (this->lightState){
    case OFF:
        if(*(this->msg)){
            light->switchOn();
            this->lightState = ON;
            Serial.println("LED Acceso!");
        }
    break;
    case ON:
        if(!*(this->msg)){
            light->switchOff();
            this->lightState = OFF;
            Serial.println("LED Spento!");
        }
    break;
    default:
    break;
    }
}