
#include <string>

#include "executor.h"
#include "listener.h"
#include "thread.h"

#include <sys/socket.h>

using namespace std;

NewConnection::NewConnection(int sockfd):
    Task("connection"), _sockfd(sockfd) {}

void
NewConnection::operator()() {
}

void*
Listener::_proc(void* context) {
    Listener* listener = static_cast<Listener*>(context);
    return NULL;
}


Listener::Listener(const string& iface, int port, Executor* executor):
    Thread("listener", Listener::_proc, this),
    _iface(iface),
    _port(port),
    _executor(executor) {}
