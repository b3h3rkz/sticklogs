#include <stdexcept>
#include "db_wrapper.h"
#include "transaction.h"

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

bool DBWrapper::insert_transaction(const Transaction& tx) {
    std::vector<char> serialized = tx.serialize();
    rocksdb::Status status = db->Put(rocksdb::WriteOptions(), tx.id(), rocksdb::Slice(serialized.data(), serialized.size()));
    return status.ok();
}

Transaction DBWrapper::get_transaction(const std::string& id) {
    std::string value;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), id, &value);
    if (!status.ok()) {
        throw std::runtime_error("Failed to get transaction: " + status.ToString());
    }
    return Transaction::deserialize(value.data(), value.size());
}

std::vector<Transaction> DBWrapper::get_transactions_by_date_range(int64_t start_timestamp, int64_t end_timestamp) {
    std::vector<Transaction> result;
    rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        Transaction tx = Transaction::deserialize(it->value().data(), it->value().size());
        if (tx.timestamp() >= start_timestamp && tx.timestamp() <= end_timestamp) {
            result.push_back(tx);
        }
    }
    delete it;
    return result;
}

bool DBWrapper::bulk_insert_transactions(const std::vector<Transaction>& transactions) {
    rocksdb::WriteBatch batch;
    for (const auto& tx : transactions) {
        std::vector<char> serialized = tx.serialize();
        batch.Put(tx.id(), rocksdb::Slice(serialized.data(), serialized.size()));
    }
    
    rocksdb::Status status = db->Write(rocksdb::WriteOptions(), &batch);
    return status.ok();
}