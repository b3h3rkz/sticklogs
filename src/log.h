#ifndef LOG_H
#define LOG_H

#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

class Log {
public:
    Log(std::string reference, nlohmann::json metadata, int64_t timestamp = 0);

    std::string reference() const;
    nlohmann::json metadata() const;
    int64_t timestamp() const;

    std::string serialize() const;
    static Log deserialize(const std::string& data);

private:
    std::string m_reference;
    nlohmann::json m_metadata;
    int64_t m_timestamp;
};

#endif // LOG_H