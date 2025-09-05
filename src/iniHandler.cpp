#include "iniHandler.h"


/* @brief ini 파일을 읽는 함수
*  
*/
bool read_ini(const std::string ini_path, std::unordered_map<std::string, std::string>& ini_result) {
    std::ifstream ini_str(ini_path);

    if (!ini_str.is_open()) {
        std::cerr << "Error : Cannot open ini file" <<std::endl;
        return false;
    }

    std::string line;
    std::unordered_map<std::string, std::string> ini;
    std::string key;
    std::string value;
    while (std::getline(ini_str, line)) {
        if (start_with(line, "//") || start_with(line, "#") || start_with(line, "/") || line.empty()) {
            continue;
        }
        else if (start_with(line, "[")) {
            continue;
        }
        else {
            std::vector<std::string> tokens;
            char sep;
            strip(line);
            if (line.find(':') != std::string::npos ){
                sep = ':';
            } else if (line.find('=') != std::string::npos) {
                sep = '=';
            } else {
                std::cerr << "Please check your ini file to stylized in proper style";
                return false;
            }
            split(line, tokens, sep);
            key = tokens[0];
            value = tokens[1];
            strip(key);
            strip(value);
            ini[key] = value;
        }
    }

    ini_result = ini;
    return true;

}

bool configurate_roi_with_ini(const std::unordered_map<std::string, std::string>& ini, std::vector<cv::Rect>& roi_vector) {
    int row = std::stoi(ini.at("window_row"));
    int col = std::stoi(ini.at("window_col"));
    int width = std::stoi(ini.at("window_width"));
    int height = std::stoi(ini.at("window_height"));

    int num_roi = row * col;

    int cell_width = int(width / col);
    int cell_height = int(height / row);


    for (int i = 0; i < num_roi; i++) {
        cv::Rect cell(cell_width * (i % col), cell_height * (i / col), cell_width, cell_height);
        roi_vector.push_back(cell);
    }
    return true;
}