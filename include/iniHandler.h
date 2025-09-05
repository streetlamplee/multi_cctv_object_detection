#pragma once

#include "Handler.h"
#include <opencv2/opencv.hpp>

bool read_ini(const std::string ini_path, std::unordered_map<std::string, std::string>& ini_result);

bool configurate_roi_with_ini(const std::unordered_map<std::string, std::string>& ini, std::vector<cv::Rect>& roi_vector);