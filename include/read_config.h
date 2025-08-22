#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <filesystem>

bool start_with(const std::string str, const std::string prefix);

int split(const std::string& line, std::vector<std::string>& tokens, char seperator);

int strip(std::string& word);

int config_processor(std::string line, std::vector<std::string>& output);

int read_config(std::string config_path, std::unordered_map<std::string, std::vector<int>>& config);
