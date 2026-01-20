#pragma once

#include <time.h>
#include <ctime>
#include <vector>

struct detected_history_item
{
    std::time_t time;
    std::vector<int> detected_classes;
};