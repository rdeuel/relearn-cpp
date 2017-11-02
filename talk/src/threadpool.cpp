
#include <sstream>
#include <string>
#include <utility>

#include <pthread.h>

#include "spdlog/spdlog.h"

#include "logger.h"
#include "threadpool.h"

using namespace std;

/**
 * implementations
 */

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

Threadpool::Threadpool(int size, int max_pending):
     _max_pending(max_pending), _exiting(false) {
    for (int i = 0; i < size; ++i) {
        ostringstream name;
        name << "thread-" << i;
        Thread t = Thread(name.str(), _proc, (void*)this);
        _threads.push_back(move(t));
    }
    pthread_mutex_init(&_lock, NULL);
    pthread_cond_init(&_is_accepting_new_tasks, NULL);
    pthread_cond_init(&_has_pending_tasks, NULL);
}

Threadpool::~Threadpool() {
    pthread_mutex_lock(&_lock);
    _exiting = true;
    pthread_cond_broadcast(&_has_pending_tasks);
    pthread_mutex_unlock(&_lock);
    for (vector<Thread>::iterator it = _threads.begin();
         it != _threads.end();
         ++it) {
        it->join();
    }
    pthread_mutex_destroy(&_lock);
    pthread_cond_destroy(&_is_accepting_new_tasks);
    pthread_cond_destroy(&_has_pending_tasks);
}

void
Threadpool::start() {
    for (vector<Thread>::iterator it = _threads.begin();
         it != _threads.end();
         ++it) {
        it->start();
    }
}

void
Threadpool::submit(Task* task) {
    pthread_mutex_lock(&_lock);
    if (_exiting) {
        throw string("Exit requested, can't accept new tasks");
    }
    while(_tasks.size() >= _max_pending) {
        pthread_cond_wait(&_is_accepting_new_tasks, &_lock);
    }
    LOG->debug("pushing task");
    _tasks.push_front(task);
    pthread_cond_signal(&_has_pending_tasks);
    if (_tasks.size() < _max_pending) {
        pthread_cond_signal(&_is_accepting_new_tasks);
    }
    pthread_mutex_unlock(&_lock);
}

void*
Threadpool::_proc(void* context) {
    LOG->debug("entering _proc");
    Threadpool* threadpool = (Threadpool*)context;

    while(1) {
        pthread_mutex_lock(&threadpool->_lock);
        while (!threadpool->_exiting &&
               threadpool->_tasks.size() == 0) {
            pthread_cond_wait(&threadpool->_has_pending_tasks,
                              &threadpool->_lock);
        }
        LOG->debug("proc ready");
        if (threadpool->_exiting) {
            LOG->debug("exiting");
            pthread_mutex_unlock(&threadpool->_lock);
            return NULL;
        } else if (threadpool->_tasks.size() != 0) {
            Task* task = threadpool->_tasks.back();
            threadpool->_tasks.pop_back();
            pthread_cond_signal(&threadpool->_is_accepting_new_tasks);
            pthread_mutex_unlock(&threadpool->_lock);
            LOG->debug("running task");
            (*task)();
        }
    }
}

