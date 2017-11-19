
#ifndef _EXECUTOR_H
#define _EXECUTOR_H

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

class Executor {
public:
    virtual void submit(Task* task) = 0;
    virtual ~Executor(){};
};
#endif // _EXECUTOR_H
