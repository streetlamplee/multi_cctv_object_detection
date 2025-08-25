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
    word.erase(0, word.find_first_not_of(" [\t\n\r\f\v"));
    word.erase(word.find_last_not_of(" ]\t\n\r\f\v") + 1);
    return 1;
}

int make_space(std::string& str) {
    std::stringstream ss(str);
    std::string brackets = " []{}()";
    std::string str_result = "";
    char c;

    while (ss.get(c)) {
        if (brackets.find(c) != std::string::npos && !isspace(c)) {     // 괄호 중 한개라면
                str_result += ' ';
                str_result += c;
                str_result += ' ';
        }
        str_result += c;
    }
    str = str_result;
    return 1;
}

int postfix(std::string& str) {
    std::stringstream ss(str);
    std::string token;
    std::stack<std::string> op;
    std::stringstream output;
    while (ss >> token) {
        if (isdigit(token[0])) {
            output << token << " ";
        }
        else if (token == "and" || token == "or" || token == "(" || token == "{" || token == "[") {
            op.push(token);
        }
        else if (token == ")") {
            while (!op.empty() && op.top() != "(") {
                output << op.top() << " ";
                op.pop();
            }
        }
        else if (token == "}") {
            while (!op.empty() && op.top() != "{") {
                output << op.top() << " ";
                op.pop();
            }
        }
        else if (token == "]") {
            while (!op.empty() && op.top() != "[") {
                output << op.top() << " ";
                op.pop();
            }
        }
    }

    while (!op.empty()) {
        output << op.top() << " ";
        op.pop();
    }

    std::string output_str = output.str();
    if (!output_str.empty()){
        output_str.pop_back();
    }
    str = output_str;
    return 1;
}

int read_conf(std::string config_path, std::vector<Alarm>& alarms) {
    std::ifstream conf(config_path);

    if (!conf.is_open()) {
        std::cerr << "Error: Cannot Open Config File" << std::endl;
        return -1;
    }
    Alarm a;
    std::string line;
    while (std::getline(conf, line)) {
        if (start_with(line, "//") || start_with(line, "#") || start_with(line, "/")) {
            continue;
        }

        // alarm conf 끝일 경우,
        if (start_with(line, "[/")){
            if (line.find(a.get_description(), 0) == 2){
                alarms.push_back(a);
            } else {
                std::cerr << "Error : conf 파일 중, 정확하지 않은 End of Parser가 존재합니다." << std::endl;
            }
        }
        // alarm conf 시작할 경우
        else if (start_with(line, "[")){
            a = Alarm();
            std::string desc = line;
            strip(desc);
            a.set_description(desc);
        }

        else if (start_with(line, "risk_level")){
            std::vector<std::string> tokens;
            split(line, tokens, ':');
            a.set_risk_level(std::stoi(tokens[1]));
        }

        else {
            make_space(line);
            postfix(line);
            a.set_condition(line);
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