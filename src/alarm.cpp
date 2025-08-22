#include "alarm.h"

Alarm::Alarm() {}
Alarm::Alarm(std::string des, std::vector<int> cond, int r_lv) {
    this->description = des;
    this->condition = cond;
    this->risk_level = r_lv;
}
Alarm::~Alarm() {}

void Alarm::set_description(std::string des){
    this->description = des;
}
void Alarm::set_condition(std::vector<int> cond){
    this->condition = cond;
}
void Alarm::set_risk_level(int r_lv){
    this->risk_level = r_lv;
}

std::string Alarm::get_description(){
    return this->description;
}
std::vector<int> Alarm::get_condition(){
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