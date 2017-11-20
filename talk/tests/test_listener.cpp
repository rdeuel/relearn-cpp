
#include "gtest/gtest.h"
#include "logger.h"

#include <vector>

#include "executor.h"
#include "listener.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
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
    vector<char> operator()(const string& client_addr,
                            const vector<char>& req) {
        vector<char> resp(req);
        return resp;
    }
};

class ListenerTest: public ::testing::Test {
public:
    const char* _data = "this is data";
    virtual void SetUp() {
        LOG->debug("Running SetUp for ListenerTest");
    }
};

TEST_F(ListenerTest, all_tasks_complete) {
    EchoHandler handler;
    RunNowExecutor exec;
    Listener listener("0.0.0.0", 8080, exec, handler);
    listener.start();

    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = (char*)"Hello from client";
    char buffer[1024] = {0};
    sock = socket(AF_INET, SOCK_STREAM, 0);
	ASSERT_GE(sock, 0);
  
    memset(&serv_addr, '0', sizeof(serv_addr));
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
      
    // Convert IPv4 and IPv6 addresses from text to binary form
    ASSERT_GT(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr), 0);
  
    ASSERT_GE(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)), 0);
    send(sock , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");
    valread = read( sock , buffer, 1024);
	close(sock);
    printf("%s\n",buffer );
	listener.join();
}
