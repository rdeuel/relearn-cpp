
#include "httphandler.h"
#include "logger.h"

#include <string>
#include <sstream>

using namespace std;

MessageHandler::Status
HttpHandler::operator()(const string& client_addr,
                        const vector<char>& req,
                        vector<char>& resp) {
    istringstream is;
    is.rdbuf()->pubsetbuf((char *)req.data(), req.size());
    string topline;
    getline(is, topline);
    LOG->debug("Top line = {}", topline);
    is.rdbuf()->pubsetbuf(0, 0);

    char buf[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 76\r\n"
        "\r\n"
        "<html><head><title>Hello World!</title></head><body>Hello World!</body></html>";
    resp.assign(buf, buf + sizeof(buf));
    return Status::Close;
}
