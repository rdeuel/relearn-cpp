#include "spdlog/spdlog.h"

#include <iostream>
#include <sstream>

#include <unistd.h>

#include "httphandler.h"
#include "threadpool.h"
#include "logger.h"

using namespace std;

/**
 * test
 */

class PrintTask: public Task {
public:
    PrintTask(const string& name): Task(name){
        LOG->debug("constructing {}", name);
    }
    virtual void operator()(){
        LOG->debug("This is task {}", name());
    }
};

int main() {
    console_log_init();
    LOG->info("yeah that's right");
    Threadpool executor (10, 100);
    HttpHandler handler;
    Listener listener("0.0.0.0", 8088, executor, handler);
    executor.start();
    listener.start();
    while (true) {
        sleep(1);
    }
    listener.join();
}
/*
    //auto console = spdlog::stdout_color_mt("console");
    //console->info("Welcome to spdlog!");
    console_log_init();
    LOG->info("yeah that's right");
    Threadpool tp(5, 100);
    tp.start();
    for (int i = 0; i < 5; ++i) {
        ostringstream s;
        s << i;
        PrintTask* pt = new PrintTask(s.str());
        tp.submit(pt);
    }
    sleep(3);
*
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
