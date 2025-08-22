#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Alarm{
public:
    Alarm();
    Alarm(std::string des, std::vector<int> cond, int r_lv);
    ~Alarm();

    void set_description(std::string des);
    void set_condition(std::vector<int> cond);
    void set_risk_level(int r_lv);

    std::string get_description();
    std::vector<int> get_condition();
    int get_risk_level();

private:
    std::string description;
    std::vector<int> condition;
    int risk_level;

};

bool define_alarm (const std::vector<int>& AlarmSetting, const std::vector<int>& detectedClass);