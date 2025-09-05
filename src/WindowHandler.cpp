#include "Windowhandler.h"

bool paint(const std::unordered_map<std::string, std::string> inifile, cv::Mat& canvas) {
    if (inifile.empty()) {
        std::cerr << "[WindowHandler.h] inifile is empty" << std::endl;
        return false;
    }
    if (canvas.cols == 0 || canvas.rows == 0) {
        std::cerr << "[WindowHandler.h] canvas size is invalid" << std::endl;
        return false;
    }

    canvas = cv::Mat(std::stoi(inifile.at("window_height")), std::stoi(inifile.at("window_width")), CV_8UC3, cv::Scalar(0,0,0));


    return true;
}