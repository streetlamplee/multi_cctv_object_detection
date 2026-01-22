#include "server.h"
#include <nlohmann/json.hpp>

Server::Server()
{
    this->svr.Post("/api/robot-control", [](const httplib::Request &req, httplib::Response &res)
    {
        auto j = nlohmann::json::parse(req.body);
        int ch = j["channel"];
        bool mute = j["muteSignal"];
        if (ch < 0)
        {   
        }
        if (ch >= MAX_CHANNEL_NUM)
        {
        }
        else 
        {
        mute_robot_signals[ch] = mute;
        std::cout << "채널 " << ch << " 로봇 신호 차단 여부: " << mute << std::endl;
        res.set_content("Success", "text/plain");
        }
    });
    this->thr = std::thread(&Server::loop, this);
    std::cout << "[SERVER] 서버 초기화 및 실행 완료" << std::endl;
}

Server::~Server()
{
}

void Server::loop()
{
    this->svr.listen("0.0.0.0", 8081);
}
