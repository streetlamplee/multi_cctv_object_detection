#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <sstream>
#include <chrono>
#include <time.h>
#include <iomanip>
#include <iostream>

inline std::stringstream time_stamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&t);

    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y_%m_%d__%H_%M_%S");
    return ss;
}

inline std::string time_stamp_str() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&t);

    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y/%m/%d %H:%M:%S");
    
    return ss.str();
}