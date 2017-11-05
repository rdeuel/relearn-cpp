#include <atomic>
#include <list>
#include <string>

#include <time.h>

#include "gtest/gtest.h"
#include "logger.h"
#include "threadpool.h"

using namespace std;

class ThreadpoolTest: public ::testing::Test {
public:
    const char* _data = "this is data";
    virtual void SetUp() {
        LOG->debug("Running SetUp for ThreadPoolTest");
    }
};

void delayms(int milliseconds) {
    struct timespec delay;
    delay.tv_sec = milliseconds / 1000;
    delay.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&delay, &delay);
}

class TestTask: public Task {
    atomic<bool> _done;
    const int _task_number;
    const long _milliseconds;

public:
    TestTask(int task_number, long milliseconds):
        Task(string("task " + to_string(task_number))), _done(false),
         _task_number(task_number), _milliseconds(milliseconds) {
        LOG->debug("constructing task {}", _task_number);
    }
    virtual void operator()(){
        LOG->debug("starting task {}", _task_number);
        delayms(_milliseconds);
        LOG->debug("finishing task {}", _task_number);
        _done = true;
    }
    int task_number() const {
        return _task_number;
    }
    bool done() const {
        return bool(_done);
    }
};

TEST_F(ThreadpoolTest, all_tasks_complete) {
    LOG->debug("this is test 1: {}", _data);
    Threadpool tp(4, 2);
    tp.start();
    int num_tasks = 10;
    list<TestTask> tasks;
    for (int i = 0; i < num_tasks; ++i) {
        tasks.emplace_back(i, 2000);
        tp.submit(&tasks.back());
    }

    while (true) {
        int done_count = 0;
        for (list<TestTask>::iterator it = tasks.begin();
             it != tasks.end();
             ++it) {
            if (it->done()) {
                done_count++;
            }
        }
        if (done_count == num_tasks) {
            LOG->debug("All done");
            break;
        } else {
            LOG->debug("done count = {}, sleeping...", done_count);
            delayms(1000);
        }
    }
}
