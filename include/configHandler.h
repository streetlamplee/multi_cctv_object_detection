#pragma once

#include "Handler.h"
#include "json.hpp"
using json = nlohmann::json;
#include "alarm.h"
#include <stack>


int read_config_json(std::string json_path, json& config);



int read_conf(std::string config_path, std::vector<Alarm>& alarms);