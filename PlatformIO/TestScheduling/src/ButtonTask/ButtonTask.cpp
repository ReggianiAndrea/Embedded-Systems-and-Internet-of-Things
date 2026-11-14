#include <Arduino.h>
#include "ButtonTask.h"
#include "../Button/ButtonImpl.h"

ButtonTask::ButtonTask(int pin,bool* msg){
    this->msg=msg;
    this->pin=pin;
}

void ButtonTask::init(int period){
    Task::init(period);
    this->btn = new ButtonImpl(this->pin);
    this->btnState = RELEASED;
}

void ButtonTask::tick(){
    switch(this->btnState){
        case RELEASED:
            if(btn->isPressed()){
                btnState = PRESSED;
                *msg=!*msg;
                Serial.println("Bottone appena premuto!");
            }
        break;
        case PRESSED:
            if(!btn->isPressed()){
                btnState=RELEASED;
                //*msg=false;
                Serial.println("Bottone appena rilasciato!");
            }
        break ;
    }
}
