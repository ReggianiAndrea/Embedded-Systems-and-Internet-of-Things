#ifndef __SCHEDULER__
#define __SCHEDULER__

//#include "Timer.h"
#include "../Task/Task.h"
#include "../Timer/Timer.h"

#define MAX_TASKS 10
class Scheduler {

 int basePeriod;//periodo base dello schduler
 int nTasks;
 Task* taskList[MAX_TASKS];
 Timer timer;
 
public:

 void init(int basePeriod);
 virtual bool addTask(Task* task);
 virtual void schedule();

};
#endif