#include "server.h"
#include <nlohmann/json.hpp>

Server::Server()
{
    this->svr.Post("/api/robot-control", [](const httplib::Request &req, httplib::Response &res)
                   {
    // ★ CORS 헤더 추가
    res.set_header("Access-Control-Allow-Origin", "*");
    
    try {
        auto j = nlohmann::json::parse(req.body);
        int ch = j["channel"];
        bool mute = j["muteSignal"];
        
        if (ch < 1 || ch > MAX_CHANNEL_NUM)
        {
            std::cerr << "[SERVER] 잘못된 채널 번호: " << ch << std::endl;
            res.status = 400;
            res.set_content("Invalid channel", "text/plain");
            return;
        }
        
        mute_robot_signals[ch] = mute;
        std::cout << "[SERVER] 채널 " << ch << " 로봇 신호 차단: " << mute << std::endl;
        res.set_content("Success", "text/plain");
    }
    catch (const std::exception& e) {
        std::cerr << "[SERVER] 요청 처리 중 오류: " << e.what() << std::endl;
        res.status = 400;
        res.set_content("Bad request", "text/plain");
    } });

    this->svr.Get("/api/robot-control", [](const httplib::Request &req, httplib::Response &res)
                  {
    res.set_header("Access-Control-Allow-Origin", "*");
    
    nlohmann::json j;
    j["states"] = nlohmann::json::array();
    
    // 1-based 인덱싱 유지 (다른 코드와 일관성)
    for (int ch = 1; ch <= MAX_CHANNEL_NUM; ch++)
    {
        j["states"].push_back({
            {"channel", ch},
            {"muted", (bool)mute_robot_signals[ch]}
        });
    }
    
    res.set_content(j.dump(), "application/json"); });

    // ★ CORS preflight(OPTIONS) 대응
    this->svr.Options("/api/robot-control", [](const httplib::Request &, httplib::Response &res)
                      {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type"); });
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
