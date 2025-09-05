#include "configHandler.h"



int read_conf(std::string config_path, std::vector<Alarm>& alarms) {
    std::ifstream conf(config_path);

    if (!conf.is_open()) {
        std::cerr << "Error: Cannot Open Config File" << std::endl;
        return -1;
    }
    Alarm* a = nullptr;
    std::string line;
    while (std::getline(conf, line)) {
        if (start_with(line, "//") || start_with(line, "#") || start_with(line, "/") || line.empty()) {
            continue;
        }

        // alarm conf 끝일 경우,        *피드백 : conf 종료 양식 삭제
        // if (start_with(line, "[/")){
        //     if (line.find(a->get_description(), 0) == 2){
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
            a = new Alarm();
            std::string desc = line;
            strip(desc);
            a->set_description(desc);
        }

        else if (start_with(line, "risk_level")){
            std::vector<std::string> tokens;
            split(line, tokens, ':');
            a->set_risk_level(std::stoi(tokens[1]));
        }

        else {
            make_space(line);
            postfix(line);
            a->set_condition(line);
        }
        
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