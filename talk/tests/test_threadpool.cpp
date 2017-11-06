#include <atomic>
#include <list>
#include <string>

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "gtest/gtest.h"
#include "logger.h"
#include "threadpool.h"

using namespace std;

class TestTask;

void
delayms(int milliseconds) {
    struct timespec delay;
    delay.tv_sec = milliseconds / 1000;
    delay.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&delay, &delay);
}

long
curr_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

long
random_ms(long min, long max) {
    int r = rand();
    return r % (max - min) + min;
}

class TestTask: public Task {
    atomic<bool> _done;
    const int _task_number;
    const long _milliseconds;

public:
    TestTask(int task_number, long milliseconds):
        Task(string("task " + to_string(task_number))), _done(false),
         _task_number(task_number), _milliseconds(milliseconds) {
        srand(time(NULL));
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
    static int count_done(const list<TestTask>& tasks) {
        int count = 0;
        for (list<TestTask>::const_iterator it = tasks.begin();
             it != tasks.end();
             ++it) {
            if (it->done()) {
                count++;
            }
        }
        return count;
    }
};

class ThreadpoolTest: public ::testing::Test {
public:
    const char* _data = "this is data";
    virtual void SetUp() {
        LOG->debug("Running SetUp for ThreadPoolTest");
    }

    void assert_completes(int max_running, int max_pending);
};

void
ThreadpoolTest::assert_completes(int max_running, int max_pending) {
    float longest_task = 1000.0;
    float shortest_task = 0.0;
    float average_task = (longest_task - shortest_task) / 2;
    int num_tasks = 1000;
    float expected_complete = (num_tasks * average_task) / max_running;
    LOG->info("threadpool with {} threads, expected complete in {}ms.",
              max_running, expected_complete);

    // start timing
    long start_time = curr_milliseconds();

    Threadpool tp(max_running, max_pending);
    tp.start();
    list<TestTask> tasks;
    for (int i = 0; i < num_tasks; ++i) {
        tasks.emplace_back(i, random_ms(shortest_task, longest_task));
        tp.submit(&tasks.back());
        ASSERT_LE(tp.num_pending(), max_pending);
    }

    // delay for whatever time is left from the expected time, plus a fudge
    // of 10ms. We should be all done at that point.
    long since_start = curr_milliseconds() - start_time;
    delayms(int(expected_complete) - since_start + 10);
    ASSERT_EQ(num_tasks, TestTask::count_done(tasks));
}

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
        int num_done = TestTask::count_done(tasks);
        if (num_done == num_tasks) {
            LOG->debug("All done");
            break;
        } else {
            LOG->debug("done count = {}, sleeping...", num_done);
            delayms(1000);
        }
    }
}

TEST_F(ThreadpoolTest, test_8t2p) {
    assert_completes(8, 2);
}

TEST_F(ThreadpoolTest, test_20t10p) {
    assert_completes(20, 10);
}

TEST_F(ThreadpoolTest, test_50t25p) {
    assert_completes(50, 25);
}

TEST_F(ThreadpoolTest, test_1t500p) {
    assert_completes(1, 5);
}
