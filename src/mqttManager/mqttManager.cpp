#include "mqttManager/mqttManager.h"
#include <iostream>


MqttManager::~MqttManager()
{
    disconnect();
    delete client_;
}

void MqttManager::connect() {
    if (client_)
    {
        std::cout << "[MQTT] Already connected" << std::endl;
        return;
    }

    try
    {
        this->client_ = new mqtt::async_client(this->address, this->clientID);
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        connOpts.set_user_name(this->userID);
        connOpts.set_password(this->password);

        std::cout << "[MQTT] 브로커 연결 시도" << std::endl;
        client_->connect(connOpts)->wait();
        std::cout << "[MQTT] 연결 성공" << std::endl;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "[MQTT]] 연결 실패" << std::endl;
    }
}

void MqttManager::connect(const std::string &address, const std::string &clientID, const std::string &userID, const std::string &password)
{
    if (client_)
    {
        std::cout << "[MQTT] Already connected" << std::endl;
        return;
    }

    this->address = address;
    this->clientID = clientID;
    this->userID = userID;
    this->password = password;

    try
    {
        this->client_ = new mqtt::async_client(address, clientID);
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        connOpts.set_user_name(userID);
        connOpts.set_password(password);

        connOpts.set_automatic_reconnect(1, 10);

        std::cout << "[MQTT] 브로커 연결 시도" << std::endl;
        client_->connect(connOpts)->wait();
        std::cout << "[MQTT] 연결 성공" << std::endl;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "[MQTT]] 연결 실패" << std::endl;
    }
}

void MqttManager::disconnect()
{
    if (client_ && client_->is_connected())
    {
        try
        {
            client_->disconnect()->wait();
            std::cout << "[MQTT] 연결 종료." << std::endl;
        }
        catch (...)
        {
        }
    }
}

void MqttManager::publish(const std::string &topic, const nlohmann::json &payload)
{
    if (!client_ || !client_->is_connected())
    {
        std::cerr << "[MQTT] 오류: 연결되지 않아서 보낼 수 없습니다." << std::endl;
        return;
    }

    // camera_id가 있을 때만 mute 체크 수행
    if (payload.contains("camera_id") && payload["camera_id"].is_number())
    {
        int cam_id = payload["camera_id"].get<int>();
        constexpr int MUTE_ARR_SIZE = sizeof(mute_robot_signals) / sizeof(mute_robot_signals[0]);
        if (cam_id >= 0 && cam_id < MUTE_ARR_SIZE && mute_robot_signals[cam_id])
        {
            return;
        }
    }

    try
    {
        std::string pl = payload.dump();
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, pl);
        pubmsg->set_qos(1);
        client_->publish(pubmsg)->wait_for(std::chrono::seconds(2));
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "[MQTT] 전송 실패: " << exc.what() << std::endl;
    }
}