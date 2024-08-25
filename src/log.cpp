#include "log.h"
#include <chrono>

Log::Log(std::string reference, nlohmann::json metadata, int64_t timestamp)
    : m_reference(std::move(reference)), m_metadata(std::move(metadata)) {
    if (timestamp == 0) {
        m_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    } else {
        m_timestamp = timestamp;
    }
}

std::string Log::reference() const { return m_reference; }
nlohmann::json Log::metadata() const { return m_metadata; }
int64_t Log::timestamp() const { return m_timestamp; }

std::string Log::serialize() const {
    nlohmann::json j = {
        {"reference", m_reference},
        {"metadata", m_metadata},
        {"timestamp", m_timestamp}
    };
    return j.dump();
}

Log Log::deserialize(const std::string& data) {
    nlohmann::json j = nlohmann::json::parse(data);
    return Log(j["reference"], j["metadata"], j["timestamp"]);
}