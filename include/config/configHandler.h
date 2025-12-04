#pragma once

#include "config/Handler.h"
#include "alarm/alarm.h"
#include <stack>


// int read_config_json(std::string json_path, json& config);



int read_conf(std::string config_path, std::vector<Alarm>& alarms);