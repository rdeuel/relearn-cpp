#ifndef _HTTP_HANDLER_H
#define _HTTP_HANDLER_H

#include <sstream>
#include <string>
#include <vector>

#include "listener.h"


class HttpHandler: public MessageHandler {
public:
     Status operator()(const string& client_addr,
                       const vector<char>& req,
                       vector<char>& resp);
};

#endif // _HTTP_HANDLER_H
