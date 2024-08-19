#include "db_wrapper.h"
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <ctime>
#include <iomanip>
#include <sstream>

DBWrapper::DBWrapper(const std::string& db_path) {
    rocksdb::Options options;
    // I took these examples from Facebooks' setup for tuning Rocksdb to improve performance
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;
    options.error_if_exists = false;
    options.keep_log_file_num = 1;
    options.max_open_files = -1; //we are only using this now because it's running on a single process

    rocksdb::DB* db_ptr = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_ptr);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open database: " + status.ToString());
    }
    db_.reset(db_ptr);
}

DBWrapper::~DBWrapper() = default;

bool DBWrapper::insert_transaction(const Transaction& transaction) {
    std::string key = generate_key(transaction);
    std::vector<char> value = transaction.serialize();
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, rocksdb::Slice(value.data(), value.size()));
    return status.ok();
}

bool DBWrapper::bulk_insert_transactions(const std::vector<Transaction>& transactions) {
    rocksdb::WriteBatch batch;
    for (const auto& transaction : transactions) {
        std::string key = generate_key(transaction);
        std::vector<char> value = transaction.serialize();
        batch.Put(key, rocksdb::Slice(value.data(), value.size()));
    }
    rocksdb::Status status = db_->Write(rocksdb::WriteOptions(), &batch);
    return status.ok();
}

std::optional<Transaction> DBWrapper::get_transaction(const std::string& id) {
    std::string value;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), id, &value);
    if (status.ok()) {
        return Transaction::deserialize(value.data(), value.size());
    }
    return std::nullopt;
}

std::vector<Transaction> DBWrapper::get_transactions_by_date_range(int64_t start_timestamp, int64_t end_timestamp) {
    std::vector<Transaction> result;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));

    std::string start_key = generate_key_from_timestamp(start_timestamp);
    for (it->Seek(start_key); it->Valid(); it->Next()) {
        Transaction tx = Transaction::deserialize(it->value().data(), it->value().size());
        if (tx.timestamp() > end_timestamp) {
            break;
        }
        result.push_back(std::move(tx));
    }

    return result;
}

std::string DBWrapper::generate_key(const Transaction& transaction) {
    return generate_key_from_timestamp(transaction.timestamp()) + ":" + transaction.id();
}

std::string DBWrapper::generate_key_from_timestamp(int64_t timestamp) {
    std::time_t time = timestamp;
    std::tm* ptm = std::localtime(&time);
    char date_buffer[32];
    std::strftime(date_buffer, 32, "%Y-%m-%d", ptm);
    return std::string(date_buffer);
}