#ifndef DB_WRAPPER_H
#define DB_WRAPPER_H

#include <string>
#include <vector>
#include <rocksdb/db.h>
#include "transaction.h"

class DBWrapper {
public:
    DBWrapper(const std::string& db_path);
    ~DBWrapper();

    bool insert_transaction(const Transaction& tx);
    Transaction get_transaction(const std::string& id);
    std::vector<Transaction> get_transactions_by_date_range(int64_t start_timestamp, int64_t end_timestamp);
    bool bulk_insert_transactions(const std::vector<Transaction>& transactions);

private:
    rocksdb::DB* db;
};

#endif // DB_WRAPPER_H