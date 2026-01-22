#pragma once

#include <time.h>
#include <ctime>
#include <vector>
#include <atomic>

#define MAX_CHANNEL_NUM 16

struct detected_history_item
{
    std::time_t time;
    std::vector<int> detected_classes;
};

extern std::atomic<bool> mute_robot_signals[MAX_CHANNEL_NUM];