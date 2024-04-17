//
// Created by helgesander on 17.04.2024.
//

module;

#include <Windows.h>
#include "antivirus_service.h"

export module Logger;

import <fstream>;
import <string>;
import <map>;

export enum LOGTYPE {
    INFO,
    WARNING,
    ERR,
    DEBUG
};

export class Logger {
public:
    explicit Logger(const std::string& path); // конструкто будет вызываться явно
    ~Logger() noexcept ;
    void write(const std::string& msg, LOGTYPE type);
private:
    std::ofstream logfile;
    std::map <LOGTYPE, std::string> logtypeNames{};
    // Private methods 
    std::string GetEnumName(LOGTYPE logtype);
};

Logger::Logger(const std::string& path) : logfile(path) {
    logtypeNames = {
            { INFO, "INFO" },
            { WARNING, "WARNING" },
            { ERR, "ERROR" },
            { DEBUG, "DEBUG" }
    };
}
Logger::~Logger() noexcept {
    if (logfile.is_open()) {
        logfile.close();
    }
};

void Logger::write(const std::string& msg, LOGTYPE type) {
    logfile << "[" << GetEnumName(type) << "] " << msg << std::endl;
}

std::string Logger::GetEnumName(LOGTYPE logtype)
{
    return logtypeNames[logtype];
}

export Logger GlobalLogger(LOGFILE);