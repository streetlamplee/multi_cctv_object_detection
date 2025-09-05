#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <stack>
#include <algorithm>

int make_space(std::string& str);

int postfix(std::string& str);

bool start_with(const std::string str, const std::string prefix);

int split(const std::string& line, std::vector<std::string>& tokens, char seperator);

int strip(std::string& s);
