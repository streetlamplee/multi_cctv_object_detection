#pragma once

#include "alarm/alarm.h"
#include "config/logHandler.h"
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>

extern Log log_handler;

// 1106 hj modbus 적용
class AlarmManager {
public:
    AlarmManager();
    ~AlarmManager();

    void load_alarms_from_file(const std::string& file_path);
    void process_channel_alarms(int channel_id, const std::vector<int>& detected_classes);
    void start_cooldown(int channel_id);
    bool is_on_cooldown(int channel_id);

    // 1106 hj modbus 적용
    int get_modbus_alarm_status_reg(int channel_id);
    int get_modbus_alarm_id_reg(int channel_id);
    int get_modbus_alarm_complete_reg(int channel_id);

private:
    std::vector<Alarm> alarms;
    std::unordered_map<int, std::chrono::steady_clock::time_point> channel_cooldowns;
    std::mutex cooldown_mutex;
};
