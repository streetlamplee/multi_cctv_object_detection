# pragma once

#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <iostream>
#include <string>

bool paint(const std::unordered_map<std::string, std::string> inifile, cv::Mat& canvas);