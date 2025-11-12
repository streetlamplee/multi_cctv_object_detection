#include "alarmManager.h"
#include "configHandler.h"
#include "_modbus/modbus_handler.h"
#include "logHandler.h"
#include <iostream>

// 1106 hj modbus 적용
AlarmManager::AlarmManager() {}

// 1106 hj modbus 적용
AlarmManager::~AlarmManager() {}

// 1106 hj modbus 적용
void AlarmManager::load_alarms_from_file(const std::string& file_path) {
    read_conf(file_path, this->alarms);

    int alarm_id_counter = 1;
    for (auto& alarm : this->alarms) {
        alarm.set_id(alarm_id_counter++);
    }
}

// 1106 hj modbus 적용
void AlarmManager::process_channel_alarms(int channel_id, const std::vector<int>& detected_classes) {
    int complete_reg = get_modbus_alarm_complete_reg(channel_id);
    int status_reg = get_modbus_alarm_status_reg(channel_id);
    int id_reg = get_modbus_alarm_id_reg(channel_id);

    if (complete_reg == -1 || status_reg == -1 || id_reg == -1) {
        // 1107 hj aarch64 compile 코드 적용
        // LogHandler::getInstance().log(LogLevel::ERROR, "Invalid channel ID for Modbus: " + std::to_string(channel_id));
        std::cout << "ERROR: Invalid channel ID for Modbus: " << channel_id << std::endl;
        return;
    }

    // 1. Master의 완료 신호 확인
    if (modbus_handler_get_hreg(complete_reg) == 1) {
        // 2. 알람 상태 초기화
        modbus_handler_set_ireg(status_reg, 0); // status = false
        modbus_handler_set_ireg(id_reg, 0);     // id = 0
        modbus_handler_set_hreg(complete_reg, 0); // 완료 신호 초기화
        // 1107 hj aarch64 compile 코드 적용
        // LogHandler::getInstance().log(LogLevel::INFO, "Alarm reset for channel " + std::to_string(channel_id));
        std::cout << "INFO: Alarm reset for channel " << channel_id << std::endl;
    }

    // 이미 알람이 활성화된 경우, 새로운 알람을 확인하지 않음
    if (modbus_handler_get_ireg(status_reg) == 1) {
        return;
    }

    // 3. & 4. 새로운 알람 확인
    for (auto& alarm : this->alarms) {
        if (alarm.get_target_channel() == channel_id) {
            if (define_alarm(alarm.get_condition(), detected_classes)) {
                // 5. Modbus에 알람 정보 전송
                modbus_handler_set_ireg(status_reg, 1); // status = true
                modbus_handler_set_ireg(id_reg, alarm.get_id());
                
                std::string log_msg = "ALARM on channel " + std::to_string(channel_id) + ": " + alarm.get_alarm_sentence();
                // 1107 hj aarch64 compile 코드 적용
                // LogHandler::getInstance().log(LogLevel::WARNING, log_msg);
                std::cout << "WARNING: " << log_msg << std::endl;

                break; 
            }
        }
    }
}

// 1110 hj modbus 적용
int AlarmManager::get_modbus_alarm_status_reg(int channel_id) {
    if (channel_id >= 1 && channel_id <= 30) {
        return MODBUS_IREG_SYSTEM_RESERVED + (channel_id - 1) * 2;
    }
    return -1; // Invalid channel
}

// 1110 hj modbus 적용
int AlarmManager::get_modbus_alarm_id_reg(int channel_id) {
    if (channel_id >= 1 && channel_id <= 30) {
        return MODBUS_IREG_SYSTEM_RESERVED + 1 + (channel_id - 1) * 2;
    } 
    return -1; // Invalid channel
}

// 1110 hj modbus 적용
int AlarmManager::get_modbus_alarm_complete_reg(int channel_id) {
    if (channel_id >= 1 && channel_id <= 30) {
        return MODBUS_HREG_SYSTEM_OPTION_RESERVED + (channel_id - 1) * 2;
    }
    return -1; // Invalid channel
}
