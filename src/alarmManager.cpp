#include "alarmManager.h"
#include "configHandler.h"
#include "_modbus/modbus_handler.h"
#include <iostream>

// 1106 hj modbus 적용
AlarmManager::AlarmManager() {}

// 1106 hj modbus 적용
AlarmManager::~AlarmManager() {}

void AlarmManager::start_cooldown(int channel_id) {
    std::lock_guard<std::mutex> lock(cooldown_mutex);
    channel_cooldowns[channel_id] = std::chrono::steady_clock::now();
}

bool AlarmManager::is_on_cooldown(int channel_id) {
    std::lock_guard<std::mutex> lock(cooldown_mutex);
    if (channel_cooldowns.count(channel_id)) {
        auto now = std::chrono::steady_clock::now();
        auto cooldown_start = channel_cooldowns.at(channel_id);
        auto cooldown_duration = std::chrono::seconds(10); // 10초 쿨다운
        if (now - cooldown_start < cooldown_duration) {
            return true; // 쿨다운 상태
        }
    }
    return false; // 쿨다운 상태 아님
}

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
    // 쿨다운 상태이거나 이미 알람이 활성화된 경우, 새로운 알람을 확인하지 않음
    if (is_on_cooldown(channel_id)) {
        return;
    }

    int status_reg = get_modbus_alarm_status_reg(channel_id);
    if (status_reg == -1) {
        log_handler.push(Log::Level::ERROR, "Invalid channel ID for Modbus status: " + std::to_string(channel_id));
        return;
    }
    if (modbus_handler_get_ireg(status_reg) == 1) {
        return;
    }

    // 새로운 알람 확인
    for (auto& alarm : this->alarms) {
        if (alarm.get_target_channel() == channel_id) {
            if (define_alarm(alarm.get_condition(), detected_classes)) {
                int id_reg = get_modbus_alarm_id_reg(channel_id);
                if (id_reg == -1) {
                    log_handler.push(Log::Level::ERROR, "Invalid channel ID for Modbus ID: " + std::to_string(channel_id));
                    break; 
                }
                // Modbus에 알람 정보 전송
                modbus_handler_set_ireg(status_reg, 1); // status = true
                modbus_handler_set_ireg(id_reg, alarm.get_alarm_id());
                
                std::string log_msg = "ALARM on channel " + std::to_string(channel_id) + ": " + alarm.get_alarm_sentence();
                log_handler.push(Log::Level::ALARM, log_msg);
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
