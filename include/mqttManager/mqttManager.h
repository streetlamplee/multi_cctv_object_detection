#pragma once

#include <string>
#include <mqtt/async_client.h>

class MqttManager {
public :
    static MqttManager& getInstance() {
        static MqttManager instance;
        return instance;
    }

    MqttManager(const MqttManager&) = delete;
    void operator=(const MqttManager&) = delete;

    void connect(const std::string& address, const std::string& clientID);
    void disconnect();
    void publish(const std::string& topic, const std::string& payload);

private:
    MqttManager() : client_(nullptr) {}
    ~MqttManager();

    mqtt::async_client* client_;
};