
#include "gtest/gtest.h"
#include "logger.h"
#include "listener.h"

using namespace std;

class ListenerTest: public ::testing::Test {
public:
    const char* _data = "this is data";
    virtual void SetUp() {
        LOG->debug("Running SetUp for ListenerTest");
    }
};

TEST_F(ListenerTest, all_tasks_complete) {
}
