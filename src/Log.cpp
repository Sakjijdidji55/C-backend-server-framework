//
// Created by HP on 2025/11/19.
//

#include "../include/Log.h"

Log* Log::instance = nullptr;

Log::Log() {
    fp = fopen(path.c_str(), "a+");
}

Log::~Log() {
    fclose(fp);
}

void Log::write(const std::string& msg) {
    fprintf(fp, "%s\n", msg.c_str());
}

Log* Log::getInstance() {
    if (instance == nullptr) {
        instance = new Log();
    }
    return instance;
}