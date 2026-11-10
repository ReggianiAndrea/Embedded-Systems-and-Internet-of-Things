#include "Button.h"

class ButtonImpl: public Button {
private:
  int pin;


public:
  ButtonImpl(int pin);
  bool isPressed();
};

