#include "alarm.h"

Alarm::Alarm() {}
Alarm::Alarm(std::string des, json cond, int r_lv) {
    this->description = des;
    this->condition = cond;
    this->risk_level = r_lv;
}
Alarm::~Alarm() {}

void Alarm::set_description(std::string des){
    this->description = des;
}
void Alarm::set_condition(json cond){
    this->condition = cond;
}
void Alarm::set_risk_level(int r_lv){
    this->risk_level = r_lv;
}

std::string Alarm::get_description(){
    return this->description;
}
json Alarm::get_condition(){
    return this->condition;
}
int Alarm::get_risk_level(){
    return this->risk_level;
}

bool define_alarm (const std::vector<int>& AlarmSetting, const std::vector<int>& detectedClass) {
    if (detectedClass.empty()) {
        return false;
    }
    if (AlarmSetting.empty()) {
        return false;
    }

    std::unordered_set<int> classElement(detectedClass.begin(), detectedClass.end());

    for (int alarmsetting : AlarmSetting) {
        if (classElement.count(alarmsetting) == 0) {
            return false;
        }
    }

    return true;
}

bool define_alarm_json(const json& AlarmSetting, const std::vector<int>& detectedClass) {
    if (detectedClass.empty()) {
        return false;
    }

    bool result = false;
    std::unordered_set<int> classElement(detectedClass.begin(), detectedClass.end());

    // or 조건 여부 파악
    for (const auto& alarm : AlarmSetting.items()) {
        json cond = alarm.value();
        std::vector<bool> sub_goal;

        // 내부 class 포함/불포함 여부
        for (const auto& c : cond.items()) {
            int target_class = std::stoi(c.key());
            bool is_alarm_if_exsit = c.value();

            if (is_alarm_if_exsit == (classElement.count(target_class) != 0)) {
                sub_goal.push_back(true);
            } else {
                sub_goal.push_back(false);
            }
        }
        if (std::find(sub_goal.begin(), sub_goal.end(), false) == sub_goal.end()) {
            result = true;
            std::cout << "Alarm Class : " << cond.flatten() << std::endl;
            std::cout << "detected Class : ";
            for (auto& e : classElement) {
                std::cout << e << ", ";
            }
            std::cout << std::endl;
        }
    }
    return result;
}