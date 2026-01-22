#include "alarm/alarmManager.h"
#include "config/configHandler.h"
#include "mqttManager/mqttManager.h"
#include <iostream>
#include "time/timestamp.h"
// #include <random>   //for create_id
#include <cstdint> //for create_id
#include <fstream>

using json = nlohmann::json;

// 1106 hj modbus 적용
AlarmManager::AlarmManager()
{
    std::ifstream file("./resource/id.bin", std::ios::binary);
    if (file.is_open())
    {
        file.read(reinterpret_cast<char *>(&this->id), sizeof(this->id));
        file.close();
    }
}

// 1106 hj modbus 적용
AlarmManager::~AlarmManager() {}

void AlarmManager::set_cooltime(int cooltime)
{
    this->cooltime = cooltime;
}

void AlarmManager::start_cooldown(int channel_id)
{
    std::lock_guard<std::mutex> lock(cooldown_mutex);
    channel_cooldowns[channel_id] = std::chrono::steady_clock::now();
}

bool AlarmManager::is_on_cooldown(int channel_id)
{
    std::lock_guard<std::mutex> lock(cooldown_mutex);
    if (channel_cooldowns.count(channel_id))
    {
        auto now = std::chrono::steady_clock::now();
        auto cooldown_start = channel_cooldowns.at(channel_id);
        auto cooldown_duration = std::chrono::minutes(3); // 180초 쿨다운
        if (now - cooldown_start < cooldown_duration)
        {
            return true; // 쿨다운 상태
        }
    }
    return false; // 쿨다운 상태 아님
}

// 1106 hj modbus 적용
void AlarmManager::load_alarms_from_file(const std::string &file_path)
{
    read_conf(file_path, this->alarms);

    int alarm_id_counter = 1;
    for (auto &alarm : this->alarms)
    {
        alarm.set_id(alarm_id_counter++);
    }
}

// 1119 hj mqtt 적용
int AlarmManager::process_channel_alarms(int channel_id,
                                         const std::vector<int> &detected_classes,
                                         const std::deque<detected_history_item> &detected_classes_history,
                                         std::string camera_description,
                                         std::string robotDestination)
{
    // 쿨다운 상태이거나 이미 알람이 활성화된 경우, 새로운 알람을 확인하지 않음
    if (is_on_cooldown(channel_id))
    {
        return -1;
    }

    // 새로운 알람 확인
    for (auto &alarm : this->alarms)
    {
        if (alarm.get_target_channel() == channel_id)
        {
            if (define_alarm(alarm.get_condition(), detected_classes_history))
            {

                json j;
                j["id"] = this->id;
                j["camera_id"] = channel_id;
                j["alarm_id"] = alarm.get_alarm_id();
                j["destination"] = robotDestination;
                j["camera_description"] = camera_description;
                j["detected_ids"] = detected_classes;
                j["situation"] = alarm.get_alarm_sentence();
                j["message"] = alarm_context[alarm.get_alarm_id() - 1];
                j["created_at"] = time_stamp_str();

                // std::string payload = j.dump();

                MqttManager::getInstance().publish("CCTV/Alarm", j);

                std::ofstream binfile("./resource/id.bin", std::ios::binary);
                if (binfile.is_open())
                {
                    binfile.write(reinterpret_cast<const char *>(&this->id), sizeof(this->id));
                    binfile.close();
                }

                this->id++;

                if (this->id == UINT32_MAX)
                {
                    this->id = 0;
                }

                std::string log_msg = alarm.get_alarm_sentence();
                log_handler.push(Log::Level::ALARM, log_msg, channel_id);
                std::cout << "WARNING: " << log_msg << std::endl;

                start_cooldown(channel_id);
                return 1;
            }
        }
    }
    return 0;
}
