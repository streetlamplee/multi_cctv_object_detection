#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <stack>

class Alarm{
public:
    Alarm();
    Alarm(int target_channel, std::string des, std::string cond, int r_lv);
    ~Alarm();

    void set_target_channel(int target_channel);
    void set_name(std::string des);
    void set_condition(std::string cond);
    void set_risk_level(int r_lv);
    void set_alarm_sentence(std::string s);
    void set_id(int id); // 1106 hj modbus 적용

    int get_target_channel();
    std::string get_name();
    std::string get_condition();
    int get_risk_level();
    std::string get_alarm_sentence();
    int get_id(); // 1106 hj modbus 적용

private:
    int id; // 1106 hj modbus 적용
    int target_channel;
    std::string name;
    std::string alarm_sentence;
    std::string condition;
    int risk_level;

};

bool define_alarm (std::string condition, const std::vector<int>& detectedClass);