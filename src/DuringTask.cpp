//
// Created by HP on 2025/12/19.
//
#include "../include/DuringTask.h"
//#include "util.h"
#include "Log.h"

std::string getFormattedDateTime() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    return std::ctime(&now_c);
}

void task() {
    static int count = 0;
    Log::getInstance()->write("Task executed: " + std::to_string(++count)
                                                + " | Time: " + getFormattedDateTime() + "\n");
}

