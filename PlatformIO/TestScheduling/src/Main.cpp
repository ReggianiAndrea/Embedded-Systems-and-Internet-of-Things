#include <Arduino.h>

#include "Scheduler/Scheduler.h"
#include "ButtonTask/ButtonTask.h"
#include "LightTask/LightTask.h"
Scheduler sched;
bool msg;

void setup() {
  Serial.begin(9600);
  sched.init(50); // Periodo base dello scheduler: 50 ms

  msg = false;

  Task* t0 = new ButtonTask(2,&msg); // Bottone collegato al pin 2
  t0->init(50);                 // Controlla il bottone ogni 50 ms
  sched.addTask(t0);

  Task* t1 = new LightTask(3,&msg);
  t1->init(50);
  sched.addTask(t1);
}

void loop() {
  sched.schedule();
}