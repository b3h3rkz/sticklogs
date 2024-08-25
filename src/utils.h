#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

inline std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

#endif // UTILS_H