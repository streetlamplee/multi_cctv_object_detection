#pragma once

#include <iostream>
#include <string>
#include "logHandler.h"
#include "httplib.h"

class fileserver {
    private:
    httplib::Server svr;
    std::string mount_point = "./resource";

    public:
    int server_init(Log& log_handler);
    int server_start(Log& log_handler);
    int server_stop(Log& log_handler);
    fileserver() = default;
    fileserver(std::string mp) : mount_point(mp) {};
    ~fileserver() = default;
};



int fileserver::server_init(Log& log_handler){
        auto ret = this->svr.set_mount_point("/", this->mount_point);
    if (!ret) {
        std::cerr << "웹 루트 폴더를 찾을 수 없습니다." << std::endl;
        log_handler.push(Log::Level::ERROR, "웹 루트 폴더를 찾을 수 없습니다.");
        return 1;
    }
    return 0;
    
}

int fileserver::server_start(Log& log_handler){
    std::cout << "파일 서버 시작" << std::endl;
    log_handler.push(Log::Level::INFO, "파일 서버 시작");
    this->svr.listen("0.0.0.0", 8080);
    return 0;
}

int fileserver::server_stop(Log& log_handler) {
    this->svr.stop();
    log_handler.push(Log::Level::INFO, "파일 서버 종료");
    return 0;
}