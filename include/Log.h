//
// Created by HP on 2025/11/19.
//

#ifndef FLIGHTSERVER_LOG_H
#define FLIGHTSERVER_LOG_H

#include <string>

class Log {
    std::string path = "../log.log";
    FILE *fp;
    static Log *instance;
    Log();
public:
    ~Log();

    void write(const std::string &msg);

    static Log *getInstance();
};

#endif //FLIGHTSERVER_LOG_H
