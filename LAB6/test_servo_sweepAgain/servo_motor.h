#ifndef __SERVO_MOTOR__
#define __SERVO_MOTOR__

class ServoMotor {
//Is like an API, 
//is declaring the firm of the functions
//the =0 assert that is an abstrract method/function.
public:
  virtual void on() = 0;
  virtual void setPosition(int angle) = 0;
  virtual void off() = 0;
};

#endif