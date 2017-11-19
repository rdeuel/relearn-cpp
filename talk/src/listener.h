#ifndef _LISTENER_H
#define _LISTENER_H

#include <memory>
#include <string>
#include <vector>


#include "executor.h"
#include "thread.h"

class NewConnection: public Task {
    int _sockfd;
public:
    NewConnection(int sockfd);
    virtual void operator()();
    virtual std::vector<char> handle_message(const std::vector<char>& req) = 0;
};


class Listener : public Thread {
    static void* _proc(void* context);
	std::string _iface;
	int _port;
	Executor* _executor;
public:
    Listener(const string& iface, int port, Executor* executor);
};
#endif // _LISTENER_H
