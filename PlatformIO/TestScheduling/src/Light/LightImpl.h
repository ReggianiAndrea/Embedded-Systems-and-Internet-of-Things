#include "Light.h"

class LightImpl : public Light {
  int pin;
  bool status;
public:
  LightImpl(int pin);

  void switchOn();
  void switchOff();
  bool getStatus();
};

