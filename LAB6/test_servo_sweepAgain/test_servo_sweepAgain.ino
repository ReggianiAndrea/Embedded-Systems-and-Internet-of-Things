#include "servo_motor_impl.h"
#include "button_impl.h"

int pos;   
int delta;
ServoMotor* pMotor;
Button* btn;

// Variabile per memorizzare lo stato precedente del bottone
bool lastButtonState = false; 

void setup() {
  Serial.begin(9600);
  pMotor = new ServoMotorImpl(9);
  btn = new ButtonImpl(2);
  pos = 0;
  delta = 1;
}

void loop() {
  // Legge il pin hardware e, se premuto, inverte lo status interno del bottone
  btn->isPressed(); 
  
  // Recupera lo stato aggiornato
  bool currentButtonState = btn->getStatus();
  
  // Se lo stato attuale è diverso da quello precedente, l'utente ha fatto un click
  if(currentButtonState != lastButtonState){
    
    pMotor->on();
    
    // Esegue lo sweep di 180 gradi (andata o ritorno in base al segno di delta)
    for (int i = 0; i < 180; i++) {
      Serial.println(pos);
      pMotor->setPosition(pos);         
      delay(2);            
      pos += delta;
    }
    
    pMotor->off();
    
    // Annulla l'ultimo incremento del ciclo for e inverte la direzione
    pos -= delta;
    delta = -delta;
    
    // Aggiorna lo storico: ora il sistema sa che il movimento è stato completato
    lastButtonState = currentButtonState;
  }
}