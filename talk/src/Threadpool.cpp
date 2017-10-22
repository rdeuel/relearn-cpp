
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <pthread.h>

using namespace std;

typedef void *(*ThreadProc)(void*);

class Thread {
    string _name;
    ThreadProc _proc;
    pthread_t _thread;
public:
    Thread(const string &name, ThreadProc proc);
    void start();
};

Thread::Thread(const string& name, ThreadProc proc):
     _name(name), _proc(proc) {}

void
Thread::start() {
    int ret = pthread_create(&_thread, NULL, _proc, (void*)&_name);
    if(ret) {
        throw string("failed to create thread");
    }
}

class Threadpool {
    vector<Thread> _threads;
    static void* _proc(void* context);

public:
    Threadpool(int size);
    void start();
};

void*
Threadpool::_proc(void* context) {
    string& name = *(string*)context;
    cout << "This is thread " << name << endl;
}

Threadpool::Threadpool(int size) {
    for (int i = 0; i < size; ++i) {
        ostringstream name;
        name << "thread-" << i;
        Thread t = Thread(name.str(), _proc);
        _threads.push_back(move(t));
    }
}

void
Threadpool::start() {
    for (vector<Thread>::iterator it = _threads.begin();
         it != _threads.end();
         ++it) {
        it->start();
    }
}

int main() {
    Threadpool tp(10);
    tp.start();
}
