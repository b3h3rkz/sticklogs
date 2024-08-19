#include "db_wrapper.h"
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>

DBWrapper::DBWrapper(const std::string& db_path) {
    rocksdb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = false;

    // Try to open the database
    rocksdb::DB* db_ptr = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_ptr);

    // If it fails due to a lock, try to remove the lock and retry
    if (!status.ok() && status.IsIOError()) {
        std::cout << "Failed to open database. Attempting to remove lock..." << std::endl;
        remove_db_lock(db_path);
        
        // Wait a bit before retrying
        std::this_thread::sleep_for(std::chrono::seconds(1));

        status = rocksdb::DB::Open(options, db_path, &db_ptr);
    }

    if (!status.ok()) {
        throw std::runtime_error("Failed to open database: " + status.ToString());
    }

    db_.reset(db_ptr);
}

DBWrapper::~DBWrapper() = default;

void DBWrapper::remove_db_lock(const std::string& db_path) {
    std::filesystem::path lock_file = std::filesystem::path(db_path) / "LOCK";
    if (std::filesystem::exists(lock_file)) {
        try {
            std::filesystem::remove(lock_file);
            std::cout << "Lock file removed successfully." << std::endl;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to remove lock file: " << e.what() << std::endl;
        }
    } else {
        std::cout << "Lock file not found." << std::endl;
    }
}

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