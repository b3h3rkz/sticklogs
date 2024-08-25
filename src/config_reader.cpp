#include "config_reader.h"

ConfigReader::ConfigReader(const std::string& config_path) {
    config = YAML::LoadFile(config_path);
}

int64_t ConfigReader::getInt64(const std::string& key, int64_t default_value) const {
    try {
        return config["rocksdb"][key].as<int64_t>();
    } catch (...) {
        return default_value;
    }
}

int ConfigReader::getInt(const std::string& key, int default_value) const {
    try {
        return config["rocksdb"][key].as<int>();
    } catch (...) {
        return default_value;
    }
}

std::string ConfigReader::getString(const std::string& key, const std::string& default_value) const {
    try {
        return config["rocksdb"][key].as<std::string>();
    } catch (...) {
        return default_value;
    }
}