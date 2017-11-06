
#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <pthread.h>

#include "thread.h"

class Task {
    std::string _name;
public:
    Task(const std::string& name): _name(name) {}
    const std::string& name() {return _name;}
    virtual void operator()() = 0;
};

class Threadpool {
    std::vector<Thread> _threads;
    std::deque<Task*> _tasks;
    const int _max_pending;
    pthread_mutex_t _lock;
    pthread_cond_t _is_accepting_new_tasks;
    pthread_cond_t _has_pending_tasks;
    static void* _proc(void* context);
    bool _exiting;

public:
    Threadpool(int size, int max_pending);
    void start();
    void submit(Task* task);
    int num_pending();
    virtual ~Threadpool();
};
#endif // _THREADPOOL_H
