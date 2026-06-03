/*
 * Logger.h
 *
 *  Created on: Apr 9, 2023
 *      Author: posixninja
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace PP {

/**
 * @enum LogLevel
 * @brief Logging severity levels
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

/**
 * @class Logger
 * @brief Simple logging utility for the purplepois0n project
 */
class Logger {
public:
    /**
     * @brief Set the minimum log level to output
     * @param level Minimum level to log
     */
    static void setLogLevel(LogLevel level) {
        s_logLevel = level;
    }

    /**
     * @brief Get the current minimum log level
     * @return Current log level
     */
    static LogLevel getLogLevel() {
        return s_logLevel;
    }

    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    static void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }

    /**
     * @brief Log an info message
     * @param message Message to log
     */
    static void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    /**
     * @brief Log a warning message
     * @param message Message to log
     */
    static void warn(const std::string& message) {
        log(LogLevel::WARN, message);
    }

    /**
     * @brief Log an error message
     * @param message Message to log
     */
    static void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

private:
    static void log(LogLevel level, const std::string& message) {
        if (level < s_logLevel) {
            return;
        }

        std::string levelStr;
        switch (level) {
            case LogLevel::DEBUG:
                levelStr = "DEBUG";
                break;
            case LogLevel::INFO:
                levelStr = "INFO ";
                break;
            case LogLevel::WARN:
                levelStr = "WARN ";
                break;
            case LogLevel::ERROR:
                levelStr = "ERROR";
                break;
        }

        std::string timestamp = getTimestamp();
        std::ostream& stream = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
        
        stream << "[" << timestamp << "] [" << levelStr << "] " << message << std::endl;
    }

    static std::string getTimestamp() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    static LogLevel s_logLevel;
};

} /* namespace PP */

#endif /* LOGGER_H_ */

