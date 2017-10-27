
#include <iostream>
#include <sstream>

#include <unistd.h>

#include "threadpool.h"

using namespace std;

/**
 * test
 */

class PrintTask: public Task {
public:
    PrintTask(const string& name): Task(name){cout << "constructing" << name << endl;}
    virtual void operator()(){cout << "This is task " << name() << endl;}
};

int main() {
    Threadpool tp(5, 100);
    tp.start();
    for (int i = 0; i < 5; ++i) {
        ostringstream s;
        s << i;
        PrintTask* pt = new PrintTask(s.str());
        tp.submit(pt);
    }
    sleep(3);
/*
    ostringstream s;
    s << 10; 
    PrintTask* pt = new PrintTask(s.str());
    Task* ppt = pt;
    (*ppt)();
    deque<Task*> deq;
    deq.push_front(ppt);
    Task* popped = deq.back();
    (*popped)();
    return 0;
*/
}
