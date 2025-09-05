#include "Handler.h"

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

int strip(std::string& s) {
    auto is_leading_char = [](unsigned char ch) {
        return std::isspace(ch) || ch == '[';
    };

    // 공백과 ']'를 제거하는 람다 함수
    auto is_trailing_char = [](unsigned char ch) {
        return std::isspace(ch) || ch == ']';
    };

    // 왼쪽 공백 및 '[' 제거
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), is_leading_char));
    
    // 오른쪽 공백 및 ']' 제거
    s.erase(std::find_if_not(s.rbegin(), s.rend(), is_trailing_char).base(), s.end());
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
        } else {
            str_result += c;
        }
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
        if (isdigit(token[0]) || token == "not") {
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
            op.pop();
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