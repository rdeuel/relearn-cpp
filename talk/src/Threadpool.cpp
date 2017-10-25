
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <pthread.h>
#include <unistd.h>

using namespace std;

/**
 * types
 */

typedef void *(*ThreadProc)(void*);

class Task {
string _name;
public:
    Task(const string& name): _name(name) {cout << "constructing Task" << endl;}
    string& name() {return _name;}
    virtual void operator()(){cout << "default operator()" << endl;}
};

class Thread {
    string _name;
    ThreadProc _proc;
    pthread_t _thread;
    void* _ctx;
public:
    Thread(const string &name, ThreadProc proc, void* ctx);
    void start();
    void join();
};

class Threadpool {
    vector<Thread> _threads;
    deque<Task*> _tasks;
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
    virtual ~Threadpool();
};

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
    cout << "pushing task" << endl;
    _tasks.push_front(task);
    pthread_cond_signal(&_has_pending_tasks);
    if (_tasks.size() < _max_pending) {
        pthread_cond_signal(&_is_accepting_new_tasks);
    }
    pthread_mutex_unlock(&_lock);
}

void*
Threadpool::_proc(void* context) {
    cout << "entering _proc" << endl;
    Threadpool* threadpool = (Threadpool*)context;

    while(1) {
        pthread_mutex_lock(&threadpool->_lock);
        while (!threadpool->_exiting &&
               threadpool->_tasks.size() == 0) {
            pthread_cond_wait(&threadpool->_has_pending_tasks,
                              &threadpool->_lock);
        }
        cout << "proc ready" << endl;
        if (threadpool->_exiting) {
            cout << "exiting" << endl;
            pthread_mutex_unlock(&threadpool->_lock);
            return NULL;
        } else if (threadpool->_tasks.size() != 0) {
            Task* task = threadpool->_tasks.back();
            threadpool->_tasks.pop_back();
            pthread_cond_signal(&threadpool->_is_accepting_new_tasks);
            pthread_mutex_unlock(&threadpool->_lock);
            cout << "running task" << endl;
            (*task)();
        }
    }
}

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
