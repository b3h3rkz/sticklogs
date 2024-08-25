#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

class ConfigReader {
public:
    ConfigReader(const std::string& config_path);

    int64_t getInt64(const std::string& key, int64_t default_value) const;
    int getInt(const std::string& key, int default_value) const;
    std::string getString(const std::string& key, const std::string& default_value) const;

private:
    YAML::Node config;
};