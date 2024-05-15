//
// Created by helgesander on 17.04.2024.
//

module;

#include <Windows.h>
#include <winbase.h>

#define LOGFILE "C:\\Users\\helgesander\\antivirus.log"

export module Logger;

import <fstream>;
import <string>;
import <map>;
import <format>;

export enum LOGTYPE {
    INFO,
    WARNING,
    ERR,
    DEBUG
};

export class Logger {
public:
    explicit Logger(const std::string& path); // конструктор будет вызываться явно
    ~Logger() noexcept ;
    void write(const std::string& msg, LOGTYPE type);
private:
    std::ofstream logfile;
    std::map <LOGTYPE, std::string> logtypeNames{};
    // Private methods 
    std::string GetEnumName(LOGTYPE logtype);
};

Logger::Logger(std::string const& path) {
    logfile.open(path.c_str(), std::ios_base::app);
    if (!logfile.is_open()) {
        throw std::runtime_error("Failed to open log file");
    }
    logtypeNames = {
            { INFO, "INFO" },
            { WARNING, "WARNING" },
            { ERR, "ERROR" },
            { DEBUG, "DEBUG" }
    };
    write("Logger is working...", INFO);
}
Logger::~Logger() noexcept {
    if (logfile.is_open()) {
        logfile.close();
    }
};

void Logger::write(const std::string& msg, LOGTYPE type) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::string temp = GetEnumName(type);
    std::string data = std::format("{:02d}.{:02d}.{} {:02d}:{:02d}:{:02d}", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    logfile << std::format("[{}] [{}] {}", GetEnumName(type), data, msg) << std::endl;
    logfile.flush();
}

std::string Logger::GetEnumName(LOGTYPE logtype)
{
    return logtypeNames[logtype];
}