#ifndef __BUTTON__
#define __BUTTON__

class Button {
//Is like an API, 
//is declaring the firm of the functions
//the =0 assert that is an abstrract method/function.
public:
  virtual void isPressed() = 0;
  virtual bool getStatus() = 0;

};

#endif