#ifndef DB_WRAPPER_H
#define DB_WRAPPER_H

#include <string>
#include <vector>
#include <rocksdb/db.h>
#include "log.h"

class DBWrapper {
public:
    DBWrapper(const std::string& db_path);
    ~DBWrapper();

    bool insert_log(const Log& log);
    Log get_log(const std::string& reference);
    std::vector<Log> get_logs_by_time_range(int64_t start_timestamp, int64_t end_timestamp);

private:
    rocksdb::DB* db;
    std::mutex db_mutex;
};

#endif // DB_WRAPPER_H