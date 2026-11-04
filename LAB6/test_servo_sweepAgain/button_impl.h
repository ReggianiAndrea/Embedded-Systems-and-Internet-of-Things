#ifndef __BUTTON_IMPL__
#define __BUTTON_IMPL__

#include "button.h"

class ButtonImpl: public Button {

public:
  ButtonImpl(int pin);

  void isPressed();
  bool getStatus();

    
private:
  int pin;
  bool statusBtn;
  //Button btn;
};

#endif