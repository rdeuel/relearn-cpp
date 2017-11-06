#ifndef _THREAD_H
#define _THREAD_H

#include <string>
#include <pthread.h>

using namespace std;

typedef void *(*ThreadProc)(void*);

class Thread {
    std::string _name;
    ThreadProc _proc;
    pthread_t _thread;
    void* _ctx;
public:
    Thread(const std::string &name, ThreadProc proc, void* ctx);
    void start();
    void join();
};

#endif // _THREAD_H
