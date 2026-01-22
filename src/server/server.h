#pragma once

#include "global.h"
#include <thread>
#include <atomic>
#include "httplib.h"

class Server
{
private:
    std::thread thr;
    httplib::Server svr;
    void loop();
public:
    Server();
    ~Server();
};