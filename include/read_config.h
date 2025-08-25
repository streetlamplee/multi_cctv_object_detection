#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "json.hpp"
using json = nlohmann::json;
#include "alarm.h"
#include <stack>


bool start_with(const std::string str, const std::string prefix);

int split(const std::string& line, std::vector<std::string>& tokens, char seperator);

int strip(std::string& word);

int read_config_json(std::string json_path, json& config);

int make_space(std::string& str);

int postfix(std::string& str);

int read_conf(std::string config_path, std::vector<Alarm>& alarms);