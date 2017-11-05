#include <string>

#include "logger.h"
#include "thread.h"

using namespace std;

Thread::Thread(const string& name, ThreadProc proc, void *ctx):
     _name(name), _proc(proc), _ctx(ctx) {}

void
Thread::start() {
    int ret = pthread_create(&_thread, NULL, _proc, _ctx);
    if(ret) {
        throw string("failed to create thread");
    }
}

void
Thread::join() {
    pthread_join(_thread, NULL);
}


