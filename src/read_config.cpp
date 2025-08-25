#include "read_config.h"

bool start_with(const std::string str, const std::string prefix) {
    if (prefix.length() > str.length()) {
        return false;
    }

    return str.rfind(prefix, 0) == 0;
}

int split(const std::string& line, std::vector<std::string>& tokens, char seperator) {
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, seperator)){
        tokens.push_back(token);
    }
    return 1;
}

int strip(std::string& word) {
    word.erase(0, word.find_first_not_of(" \t\n\r\f\v"));
    word.erase(word.find_last_not_of(" \t\n\r\f\v") + 1);
    return 1;
}

int config_processor(std::string line, std::vector<std::string>& output) {
    std::vector<std::string> split_result;
    split(line, split_result, ':');
    for (std::string word : split_result) {
        strip(word);
        output.push_back(word);
    }
    return 1;
}

int read_config(std::string config_path, std::unordered_map<std::string, std::vector<int>>& config) {
    std::cout << "Current Working Directory: " << std::filesystem::current_path() << std::endl;
    std::ifstream configFile(config_path);

    if (!configFile.is_open()) {
        std::cerr << "Error: Cannot Open Config File" << std::endl;
        return -1;
    }

    std::string line;
    std::vector<std::string> processor_output;
    std::vector<std::string> alarm_vector_s;
    std::vector<int> alarm_vector;
    while (std::getline(configFile, line)) {
        if (start_with(line, "//") || start_with(line, "#")) {
            continue;
        }

        config_processor(line, processor_output);
        split(processor_output[1], alarm_vector_s, ',');
        for (std::string v : alarm_vector_s) {
            alarm_vector.push_back(std::stoi(v));
        }
        config[processor_output[0]] = alarm_vector;

        processor_output.clear();
        alarm_vector.clear();
        alarm_vector_s.clear();
    }
    return 1;
}


int read_config_json(std::string json_path, json& config) {
    std::ifstream configFile(json_path);
    if (!configFile.is_open()) {
        std::cerr << "Error: Cannot Open Config File" << std::endl;
        return -1;
    }

    try {
        config = json::parse(configFile);
    }
    catch (json::parse_error& e) {
        std::cerr << "JSON 파싱 오류 : " << e.what() << std::endl;
        return -1;
    }

    return 1;
}