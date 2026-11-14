#ifndef __BUTTON__
#define __BUTTON__

class Button {
//Is like an API, 
//is declaring the firm of the functions
//the =0 assert that is an abstrract method/function.
public:
  virtual bool isPressed() = 0;

};

#endif