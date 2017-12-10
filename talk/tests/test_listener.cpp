
#include "gtest/gtest.h"
#include "logger.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <random>
#include <vector>

#include "executor.h"
#include "listener.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

using namespace std;

class RunNowExecutor: public Executor {
public:
    void submit(Task* task) {
        (*task)();
    }
};

class EchoHandler: public MessageHandler {
public:
     Status operator()(const string& client_addr,
                       const vector<char>& req,
                       vector<char>& resp) {
         if (req[req.size() - 1] != '\n') {
             return Status::MoreData;
         } else {
             resp = req;
             if (req[req.size() - 2] == '\n') {
                 return Status::Close;
             } else {
                 return Status::KeepAlive;
             }
         }
    }
};

class ListenerTest: public ::testing::Test {
    EchoHandler* handler;
    RunNowExecutor* executor;
    Listener* listener;
public:
    const char* _data = "this is data";
    virtual void setup(int port) {
        // not the standard SetUp() - need a different port each time
        LOG->debug("Running setup for ListenerTest");
        handler = new EchoHandler();
        executor = new RunNowExecutor();
        listener = new Listener("0.0.0.0", port, *executor, *handler);
        listener->start();
    }

    virtual void TearDown() {
        LOG->debug("Running TearDown for ListenerTest");
        listener->stop();
        listener->join();
        delete executor;
        delete handler;
        delete listener;
    }

    int retry_connect(const string& addr, int port);
    string random_string(size_t length);
};

int
ListenerTest::retry_connect(const string& address, int port) {
    bool connected = false;
    int timer = 0;
    int sock;
    struct sockaddr_in addr;

    memset(&addr, '0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (0 >= inet_pton(AF_INET, address.c_str(), &addr.sin_addr)) {
        throw runtime_error(string("can't convert address") + address);
    }

    while (!connected) { 
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw runtime_error("Failed to create socket");
        }
        int ret = connect(sock,
                          (struct sockaddr *)&addr,
                          sizeof(struct sockaddr_in));
        if (ret < 0) {
            if (errno == ECONNREFUSED) {
                if (timer >= 10) {
                    ostringstream msg;
                    msg << "Failed to connect after "
                        << timer << " seconds, aborting";
                    throw runtime_error(msg.str());
                } else {
                    sleep(2);
                    timer += 2;
                }
            } else {
                ostringstream msg;
                msg << "error " << errno
                    << " on connect: " << strerror(errno);
                throw runtime_error(msg.str());
            }
            close(sock);
        } else {
            connected = true;
        }
    }
    return sock;
}

string
ListenerTest::random_string(size_t length) {
    auto randchar = []() -> char {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    string str(length, 0);
    generate_n(str.begin(), length - 1, randchar);
    str[length - 1] = '\0';
    return str;
}

TEST_F(ListenerTest, little_echo) {
    setup(8080);
    int sock = retry_connect("127.0.0.1", 8080);
    char *hello = (char*)"Hello from client\n\n";
    send(sock , hello , strlen(hello) , 0 );

    char buffer[1024] = {0};
    int valread = recv( sock , buffer, 1024, 0);
	close(sock);
    ASSERT_STREQ(buffer, hello);
}

TEST_F(ListenerTest, big_echo) {
    setup(8082);
    size_t len = 7500;
    string req = random_string(len);
    req[len - 1] = req[len - 2] = '\n';
    int sock = retry_connect("127.0.0.1", 8082);
    send(sock, req.data(), len, 0);

    char buffer[7600] = {0};
    int valread = recv(sock, buffer, len + 100, 0);
	close(sock);
    ASSERT_STREQ(buffer, req.data());
}

TEST_F(ListenerTest, one_big_one_small) {
    setup(8083);
    size_t len1 = 7500;
    string req1 = random_string(len1 - 1);
    req1[len1 - 1] = '\n';
    size_t len2 = 50;
    string req2 = random_string(len2 - 2);
    req2[len2 - 1] = req2[len2 - 2] = '\n';
    int sock = retry_connect("127.0.0.1", 8083);

    char buffer[7600] = {0};
    send(sock, req1.data(), len1, 0);
    int valread = recv(sock, buffer, len1 + 100, 0);
    ASSERT_STREQ(buffer, req1.data());

    send(sock, req2.data(), len2, 0);
    valread = recv(sock, buffer, len1 + 100, 0);
    ASSERT_STREQ(buffer, req2.data());
    shutdown(sock, SHUT_RDWR);
    close(sock);
}
