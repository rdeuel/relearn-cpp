#ifndef _LISTENER_H
#define _LISTENER_H

#include <memory>
#include <string>
#include <vector>


#include "executor.h"
#include "thread.h"

struct MessageHandler {
    enum class Status {
        MoreData,
        KeepAlive,
        Close
    };
    virtual Status operator()(const std::string& client_addr,
                              const std::vector<char>& req,
                              std::vector<char>& resp) = 0;
};

class NewConnection: public Task {
    int _sockfd;
    const std::string _client_addr;
    MessageHandler& _handle_message;
public:
    NewConnection(int sockfd,
                  const std::string& client_addr,
                  MessageHandler& handler);
    virtual ~NewConnection();
    void operator()();
    size_t MAX_READ = 4096;
};


class Listener : public Thread {
    static void* _proc(void* context);
	std::string _bind_addr;
	int _port;
	Executor& _executor;
    MessageHandler& _handler;
    int _stop_eventfd;
public:
    Listener(const string& bind_addr, int port,
             Executor& executor, MessageHandler& handler);
    void stop();
};
#endif // _LISTENER_H
