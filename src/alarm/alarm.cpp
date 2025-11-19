#include "alarm/alarm.h"

Alarm::Alarm() {}
Alarm::Alarm(int target_channel, std::string des, std::string cond, int r_lv) {
    this->target_channel = target_channel;
    this->name = des;
    this->condition = cond;
    this->alarm_id = r_lv;
}
Alarm::~Alarm() {}
void Alarm::set_target_channel(int target_channel){
    this->target_channel = target_channel;
}
void Alarm::set_name(std::string des){
    this->name = des;
}
void Alarm::set_condition(std::string cond){
    this->condition = cond;
}
void Alarm::set_alarm_id(int r_lv){
    this->alarm_id = r_lv;
}
void Alarm::set_alarm_sentence(std::string s){
    this->alarm_sentence = s;
}
// 1106 hj modbus 적용
void Alarm::set_id(int id){
    this->id = id;
}

int Alarm::get_target_channel(){
    return this->target_channel;
}
std::string Alarm::get_name(){
    return this->name;
}
std::string Alarm::get_condition(){
    return this->condition;
}
int Alarm::get_alarm_id(){
    return this->alarm_id;
}
std::string Alarm::get_alarm_sentence(){
    return this->alarm_sentence;
}
// 1106 hj modbus 적용
int Alarm::get_id(){
    return this->id;
}

bool define_alarm (std::string condition, const std::vector<int>& detectedClass) {
    if (condition.empty()) {
        return false;
    }

    std::unordered_set<int> classElement(detectedClass.begin(), detectedClass.end());
    std::stringstream ss(condition);
    std::stack<bool> value_stack;
    std::string token;
    bool isNot = false;
    while (ss >> token) {
        if (isdigit(token[0])) {
            bool t = (classElement.count(std::stoi(token)) != 0);
            if (isNot) {
                t = !t;
            }
            value_stack.push(t);
            isNot = false;
        }
        else if (token == "and") {
            bool val2 = value_stack.top();
            value_stack.pop();
            bool val1 = value_stack.top();
            value_stack.pop();

            value_stack.push(val1 && val2);
        }
        else if (token == "or") {
            bool val2 = value_stack.top();
            value_stack.pop();
            bool val1 = value_stack.top();
            value_stack.pop();

            value_stack.push(val1 || val2);
        }
        else if (token == "not"){
            isNot = true;
        }
    }
    return value_stack.top();
}

// bool define_alarm_json(const json& AlarmSetting, const std::vector<int>& detectedClass) {
//     if (detectedClass.empty()) {
//         return false;
//     }
//
//     bool result = false;
//     std::unordered_set<int> classElement(detectedClass.begin(), detectedClass.end());
//
//     // or 조건 여부 파악
//     for (const auto& alarm : AlarmSetting.items()) {
//         json cond = alarm.value();
//         std::vector<bool> sub_goal;
//
//         // 내부 class 포함/불포함 여부
//         for (const auto& c : cond.items()) {
//             int target_class = std::stoi(c.key());
//             bool is_alarm_if_exsit = c.value();
//
//             if (is_alarm_if_exsit == (classElement.count(target_class) != 0)) {
//                 sub_goal.push_back(true);
//             } else {
//                 sub_goal.push_back(false);
//             }
//         }
//         if (std::find(sub_goal.begin(), sub_goal.end(), false) == sub_goal.end()) {
//             result = true;
//             std::cout << "Alarm Class : " << cond.flatten() << std::endl;
//             std::cout << "detected Class : ";
//             for (auto& e : classElement) {
//                 std::cout << e << ", ";
//             }
//             std::cout << std::endl;
//         }
//     }
//     return result;
// }