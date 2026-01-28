#pragma once

#include <string>
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>
#include "global.h"

class MqttManager
{
public:
    MqttManager() : client_(nullptr) {}
    ~MqttManager();

    static MqttManager &getInstance()
    {
        static MqttManager instance;
        return instance;
    }

    MqttManager(const MqttManager &) = delete;
    void operator=(const MqttManager &) = delete;
    void connect();
    void connect(const std::string &address, const std::string &clientID, const std::string &userID, const std::string &password);
    void disconnect();
    void publish(const std::string &topic, const nlohmann::json &payload);

private:
    std::string address;
    std::string clientID;
    std::string userID;
    std::string password;
    mqtt::async_client *client_;
};