#include "mqttManager/mqttManager.h"
#include <iostream>

MqttManager::~MqttManager() {
    disconnect();
    delete client_;
}

void MqttManager::connect(const std::string& address, const std::string& clientID) {
    if (client_) {
        std::cout << "[MQTT] Already connected" << std::endl;
        return;
    }

    try {
        client_ = new mqtt::async_client(address, clientID);
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);

        std::cout << "[MQTT] 브로커 연결 시도" << std::endl;
        client_->connect(connOpts)->wait();
        std::cout << "[MQTT] 연결 성공" << std::endl;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT]] 연결 실패" << std::endl;
    }
}

void MqttManager::disconnect() {
    if (client_ && client_->is_connected()) {
        try {
            client_->disconnect()->wait();
            std::cout << "[MQTT] 연결 종료." << std::endl;
        } catch (...) {}
    }
}

void MqttManager::publish(const std::string& topic, const std::string& payload) {
    if (!client_ || !client_->is_connected()) {
        std::cerr << "[MQTT] 오류: 연결되지 않아서 보낼 수 없습니다." << std::endl;
        return;
    }

    try {
        // QoS 1로 메시지 생성 및 전송
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
        pubmsg->set_qos(1);
        client_->publish(pubmsg)->wait_for(std::chrono::seconds(2)); // 2초 타임아웃
        
        // 실제로는 너무 잦은 로그는 끄는 게 좋습니다.
        // std::cout << "[MQTT] 전송 완료: " << payload << std::endl;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT] 전송 실패: " << exc.what() << std::endl;
    }
}