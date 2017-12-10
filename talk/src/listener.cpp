
#include <exception>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include "executor.h"
#include "logger.h"
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
            LOG->debug("received {} bytes", count);
            if (count < 0) {
                if (errno == ECONNRESET) {
                    // client closed the connection
                    LOG->info("Connection reset by client");
                    req.resize(num_read);
                    end_of_read = end_of_connection = true;
                } else {
                    ostringstream msg;
                    msg << "ERROR " << errno
                        << " reading from socket:" << strerror(errno);
                    throw runtime_error(msg.str());
                }
            } else if (count == 0) {
                // connection closed cleanly
                LOG->info("Connection closed");
                end_of_connection = end_of_read = true;
            } else {
                LOG->info("Read {} bytes", count);
                num_read += count;
                if (num_read == req.capacity()) {
                    // read again
                    req.resize(req.capacity() + MAX_READ);
                    LOG->debug("Resizing, new capacity = {}", req.capacity());
                } else {
                    // make sure the vector knows its size
                    req.resize(num_read);
                    end_of_read = true;
                }
            }
        } // end of read

        if (req.size() > 0) {
            LOG->debug("Received request of size {}", req.size());
            vector<char> resp;
            MessageHandler::Status status = _handle_message(_client_addr, req, resp);
            if (status == MessageHandler::Status::MoreData) {
                LOG->debug("Handler needs more from the client");
            } else {
                if (resp.size() > 0) {
                    LOG->debug("Returning response of size {}", resp.size());
                    send(_sockfd, resp.data(), req.size(), 0);
                } else {
                    LOG->debug("No response to send back.");
                }
                if (status == MessageHandler::Status::Close) {
                    LOG->debug("Handler tells us to Close, we close.");
                    end_of_connection = true;
                } else {
                    LOG->debug("Keeping connection alive");
                }
            }
        }
    } // end of connection
    shutdown(_sockfd, SHUT_RDWR);
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

    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (0 > fcntl(sockfd, F_SETFL, flags)) {
        throw runtime_error("Failed to make socket non-blocking");
    }

    int epollfd = epoll_create1(0);
    struct epoll_event stop_event, sock_event;
    stop_event.data.fd = listener->_stop_eventfd;
    stop_event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listener->_stop_eventfd, &stop_event);
    sock_event.data.fd = sockfd;
    sock_event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &sock_event);

    const int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

    listen(sockfd,5);

    bool stopping = false;
    while (!stopping) {
        int num_events = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == listener->_stop_eventfd) {
                LOG->info("Stopping");
                stopping = true;
            } else if (events[i].data.fd == sockfd) {
                struct sockaddr_in cli_addr;
                socklen_t clilen = sizeof(cli_addr);
                int newsockfd = accept(sockfd,
                                       (struct sockaddr *)&cli_addr,
                                       &clilen);
                if (newsockfd < 0) {
                    throw runtime_error("ERROR excepting connection");
                }
                char client_addr[16];
                inet_ntop(AF_INET, &cli_addr.sin_addr,
                          client_addr, sizeof(client_addr));
                LOG->info("New connection from {}", client_addr);

                NewConnection* conn = new NewConnection(newsockfd,
                                                        client_addr,
                                                        listener->_handler);
                listener->_executor.submit(conn);
            }
        }
    }
    return NULL;
}

Listener::Listener(const string& bind_addr, int port,
                   Executor& executor, MessageHandler& handler):
    Thread("listener", Listener::_proc, this),
    _bind_addr(bind_addr),
    _port(port),
    _executor(executor),
    _handler(handler) {
    _stop_eventfd = eventfd(0, EFD_NONBLOCK);
}

void
Listener::stop() {
    int64_t val = 1;
    write(_stop_eventfd, &val, sizeof(int64_t));
}
