#pragma once

#include <rocksdb/db.h>
#include "transaction.h"
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <mutex>

class DBWrapper {
public:
    DBWrapper(const std::string& db_path);
    ~DBWrapper();

    bool insert_transaction(const Transaction& transaction);
    bool bulk_insert_transactions(const std::vector<Transaction>& transactions);
    std::optional<Transaction> get_transaction(const std::string& id);
    std::vector<Transaction> get_transactions_by_date_range(int64_t start_timestamp, int64_t end_timestamp);

private:
    std::unique_ptr<rocksdb::DB> db_;
    std::string generate_key(const Transaction& transaction);
    std::string generate_key_from_timestamp(int64_t timestamp);
    static void remove_db_lock(const std::string& db_path);
    mutable std::mutex mutex_;
};