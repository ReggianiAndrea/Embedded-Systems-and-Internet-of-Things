#include "../Task/Task.h"
#include "../Light/Light.h"

class LightTask : public Task{
private: 
    int pin;
    Light* light;
    bool* msg;
    enum{ON,OFF} lightState;
    
public:
    LightTask(int pin,bool* msg);
    void init(int period);
    void tick();

};