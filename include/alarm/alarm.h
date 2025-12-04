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
    void set_alarm_id(int r_lv);
    void set_alarm_sentence(std::string s);
    void set_alarm_context(std::unordered_map<int, std::string> context);
    void set_id(int id); // 1106 hj modbus 적용

    int get_target_channel();
    std::string get_name();
    std::string get_condition();
    int get_alarm_id();
    std::string get_alarm_sentence();
    int get_id(); // 1106 hj modbus 적용

    std::unordered_map<int, std::string> alarm_context;

private:
    int id; // 1106 hj modbus 적용
    int target_channel;
    std::string name;
    std::string alarm_sentence;
    std::string condition;
    int alarm_id;

};

bool define_alarm (std::string condition, const std::vector<int>& detectedClass);