#include "configHandler.h"



int read_conf(std::string config_path, std::vector<Alarm>& alarms) {
    std::ifstream conf(config_path);

    if (!conf.is_open()) {
        std::cerr << "Error: Cannot Open Config File" << std::endl;
        return -1;
    }
    std::unique_ptr<Alarm> a = nullptr;
    std::string line;
    while (std::getline(conf, line)) {
        if (start_with(line, "//") || start_with(line, "#") || start_with(line, "/") || line.empty()) {
            continue;
        }

        // alarm conf 끝일 경우,        *피드백 : conf 종료 양식 삭제
        // if (start_with(line, "[/")){
        //     if (line.find(a->get_name(), 0) == 2){
        //         alarms.push_back(*a);
        //     } else {
        //         std::cerr << "Error : conf 파일 중, 정확하지 않은 End of Parser가 존재합니다." << std::endl;
        //     }
        // }
        // alarm conf 시작할 경우
        else if (start_with(line, "[")){
            if (a != nullptr) {
                alarms.push_back(*a);
            }
            a = std::make_unique<Alarm>();
            std::string desc = line;
            strip(desc);
            a->set_name(desc);
        }

        else if (start_with(line, "alarm_id")){
            std::vector<std::string> tokens;
            split(line, tokens, ':');
            a->set_alarm_id(std::stoi(tokens[1]));
        }

        else if (start_with(line, "target_channel")){
            std::vector<std::string> tokens;
            split(line, tokens, ':');
            a->set_target_channel(std::stoi(tokens[1]));
        }

        else if (start_with(line, "Alarm Sentence")){
            std::vector<std::string> tokens;
            split(line, tokens, ':');
            a->set_alarm_sentence(tokens[1]);
        }

        else {
            make_space(line);
            postfix(line);
            a->set_condition(line);
        }
        
    }

    if (a != nullptr) {
        alarms.push_back(*a);
    }
    return 1;

}


// int read_config_json(std::string json_path, json& config) {
//     std::ifstream configFile(json_path);
//     if (!configFile.is_open()) {
//         std::cerr << "Error: Cannot Open Config File" << std::endl;
//         return -1;
//     }
//     try {
//         config = json::parse(configFile);
//     }
//     catch (json::parse_error& e) {
//         std::cerr << "JSON 파싱 오류 : " << e.what() << std::endl;
//         return -1;
//     }
//     return 1;
// }