#include "../Task/Task.h"
#include "../Button/Button.h"

class ButtonTask : public Task{
private:
    int pin;//mi salvo anche nel task il pin associato al pin della classe obj a cui il taskl fa riferimento
    Button *btn;
    bool* msg;
    enum {PRESSED, RELEASED } btnState;

public:
    ButtonTask(int pin,bool* msg);
    void init(int period);
    void tick();
};

