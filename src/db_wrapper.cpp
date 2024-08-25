#include <stdexcept>
#include "db_wrapper.h"
#include "log.h"

DBWrapper::DBWrapper(const std::string& db_path) {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open database: " + status.ToString());
    }
}

DBWrapper::~DBWrapper() {
    delete db;
}

bool DBWrapper::insert_log(const Log& log) {
    std::string serialized = log.serialize();
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), log.reference(), serialized);
    return status.ok();
}

Log DBWrapper::get_log(const std::string& reference) {
    std::string value;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), reference, &value);
    if (!status.ok()) {
        throw std::runtime_error("Failed to get log: " + status.ToString());
    }
    return Log::deserialize(value);
}

std::vector<Log> DBWrapper::get_logs_by_time_range(int64_t start_timestamp, int64_t end_timestamp) {
    std::vector<Log> result;
    rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        Log log = Log::deserialize(it->value().ToString());
        if (log.timestamp() >= start_timestamp && log.timestamp() <= end_timestamp) {
            result.push_back(log);
        }
    }
    delete it;
    return result;
}