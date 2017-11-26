
#include <exception>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "executor.h"
#include "listener.h"
#include "thread.h"


using namespace std;

NewConnection::NewConnection(int sockfd,
                             const string& client_addr,
                             MessageHandler& handler):
    Task("connection"),
    _sockfd(sockfd),
    _client_addr(client_addr),
    _handle_message(handler) {}

NewConnection::~NewConnection() {
    close(_sockfd);
}

void
NewConnection::operator()() {
    bool end_of_connection = false;
    while (!end_of_connection) {
        vector<char> req(MAX_READ);
        bool end_of_read = false;
        size_t num_read = 0;
        while (!end_of_read) {
            int count = recv(_sockfd, req.data() + num_read, MAX_READ, 0);
            if (count < 0) {
                if (errno == ECONNRESET) {
                    // client closed the connection
                    end_of_read = end_of_connection = true;
                } else {
                    ostringstream msg;
                    msg << "ERROR " << errno
                        << " reading from socket:" << strerror(errno);
                    throw runtime_error(msg.str());
                }
            } else if (count == 0) {
                // connection closed
                end_of_connection = end_of_read = true;
            } else {
                num_read += count;
                if (num_read == req.capacity()) {
                    // read again
                    req.resize(req.capacity() + MAX_READ);
                } else {
                    // make sure the vector knows its size
                    req.resize(num_read);
                    end_of_read = true;
                }
            }
        }

        // better if return val could be any stream
        vector<char> resp = _handle_message(_client_addr, req);
        if (resp.size() > 0) {
            size_t written = send(_sockfd, resp.data(), req.capacity(), 0);
        }
    }
}

void*
Listener::_proc(void* context) {
    Listener* listener = static_cast<Listener*>(context);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw runtime_error("ERROR opening socket");
    }

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(struct sockaddr_in));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(listener->_port);
    inet_aton(listener->_bind_addr.data(), &bind_addr.sin_addr);

    if (bind(sockfd, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) < 0) {
        ostringstream msg;
        msg << "Failed to bind to "
            << listener->_bind_addr << ":" << listener->_port;
        throw runtime_error(msg.str());
    }

    listen(sockfd,5);
    
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) {
        throw runtime_error("ERROR excepting connection");
    }
    char client_addr[16];
    inet_ntop(AF_INET, &cli_addr.sin_addr, client_addr, sizeof(client_addr));

    NewConnection* conn = new NewConnection(newsockfd,
                                            client_addr,
                                            listener->_handler);
    listener->_executor.submit(conn);

    return NULL;
}

Listener::Listener(const string& bind_addr, int port,
                   Executor& executor, MessageHandler& handler):
    Thread("listener", Listener::_proc, this),
    _bind_addr(bind_addr),
    _port(port),
    _executor(executor),
    _handler(handler) {}
