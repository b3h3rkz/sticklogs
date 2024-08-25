#include <stdexcept>
#include <iostream>
#include "db_wrapper.h"
#include "config_reader.h"
#include "log.h"
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/write_batch.h>

DBWrapper::DBWrapper(const std::string& db_path, const std::string& config_path) {
    ConfigReader config(config_path);

    rocksdb::Options options;
    options.create_if_missing = true;

    // Read configuration
    options.write_buffer_size = config.getInt64("write_buffer_size", 64 * 1024 * 1024);
    options.max_write_buffer_number = config.getInt("max_write_buffer_number", 3);
    options.min_write_buffer_number_to_merge = config.getInt("min_write_buffer_number_to_merge", 1);

    std::string compression_str = config.getString("compression", "lz4");
    if (compression_str == "lz4") {
        options.compression = rocksdb::kLZ4Compression;
    } else if (compression_str == "snappy") {
        options.compression = rocksdb::kSnappyCompression;
    } else {
        options.compression = rocksdb::kNoCompression;
    }
    options.compression_opts.level = config.getInt("compression_level", 4);

    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = rocksdb::NewLRUCache(config.getInt64("block_cache_size", 256 * 1024 * 1024));
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(config.getInt("bloom_filter_bits_per_key", 10), false));
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));

    options.max_background_jobs = config.getInt("max_background_jobs", 2);
    options.bytes_per_sync = config.getInt64("bytes_per_sync", 1048576);
    options.compaction_style = rocksdb::kCompactionStyleLevel;
    options.max_background_compactions = config.getInt("max_background_compactions", 4);
    options.max_background_flushes = config.getInt("max_background_flushes", 2);

    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open database: " + status.ToString());
    }
}

DBWrapper::~DBWrapper() {
    delete db;
}

bool DBWrapper::insert_log(const Log& log) {
    std::lock_guard<std::mutex> lock(db_mutex);
    rocksdb::WriteBatch batch;
    std::string key = log.reference();
    std::string value = log.serialize();

    // First, check if the log already exists
    std::string existing_value;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &existing_value);

    if (status.ok()) {
        // Log with this reference already exists
        return false;
    } else if (!status.IsNotFound()) {
        // An error occurred
        throw std::runtime_error("Error checking for existing log: " + status.ToString());
    }

    // If we get here, the log doesn't exist, so we can insert it
    batch.Put(key, value);

    status = db->Write(rocksdb::WriteOptions(), &batch);

    if (!status.ok()) {
        throw std::runtime_error("Failed to insert log: " + status.ToString());
    }

    return true;
}

Log DBWrapper::get_log(const std::string& reference) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::string value;
    rocksdb::Status status = db->Get(rocksdb::ReadOptions(), reference, &value);
    if (!status.ok()) {
        throw std::runtime_error("Failed to get log: " + status.ToString());
    }
    return Log::deserialize(value);
}

std::vector<Log> DBWrapper::get_logs_by_time_range(int64_t start_timestamp, int64_t end_timestamp) {
    std::lock_guard<std::mutex> lock(db_mutex);
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

bool DBWrapper::batch_insert_logs(const std::vector<Log>& logs) {
    std::lock_guard<std::mutex> lock(db_mutex);
    std::cout << "Starting batch insert of " << logs.size() << " logs" << std::endl;  // Debug log
    rocksdb::WriteBatch batch;
    for (const auto& log : logs) {
        std::string serialized = log.serialize();
        batch.Put(log.reference(), serialized);
    }

    rocksdb::WriteOptions write_options;
    rocksdb::Status status = db->Write(write_options, &batch);

    if (status.ok()) {
        std::cout << "Batch insert successful" << std::endl;  // Debug log
    } else {
        std::cerr << "Batch insert failed: " << status.ToString() << std::endl;  // Debug log
    }

    return status.ok();
}

std::vector<Log> DBWrapper::get_all_logs() {
    std::vector<Log> logs;
    std::lock_guard<std::mutex> lock(db_mutex);
    
    rocksdb::ReadOptions read_options;
    std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(read_options));

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string value = it->value().ToString();
        try {
            Log log = Log::deserialize(value);
            logs.push_back(log);
        } catch (const std::exception& e) {
            std::cerr << "Error deserializing log: " << e.what() << std::endl;
        }
    }

    if (!it->status().ok()) {
        std::cerr << "Error iterating over logs: " << it->status().ToString() << std::endl;
    }

    return logs;
}