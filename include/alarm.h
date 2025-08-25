#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "json.hpp"
using json = nlohmann::json;
#include <algorithm>

class Alarm{
public:
    Alarm();
    Alarm(std::string des, json cond, int r_lv);
    ~Alarm();

    void set_description(std::string des);
    void set_condition(json cond);
    void set_risk_level(int r_lv);

    std::string get_description();
    json get_condition();
    int get_risk_level();

private:
    std::string description;
    json condition;
    int risk_level;

};

bool define_alarm (const std::vector<int>& AlarmSetting, const std::vector<int>& detectedClass);

bool define_alarm_json(const json& AlarmSetting, const std::vector<int>& detectedClass);