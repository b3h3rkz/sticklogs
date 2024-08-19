#include "transaction.h"
#include "db_wrapper.h"
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <chrono>

// Helper function to get current timestamp
int64_t get_current_timestamp() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

// to process a single transaction
bool process_transaction(DBWrapper& db, const Transaction& tx) {
    bool success = db.insert_transaction(tx);
    if (success) {
        std::cout << "Transaction saved -> " << tx.id() << std::endl;
    } else {
        std::cerr << "Unable to save transaction: " << tx.id() << std::endl;
    }
    return success;
}

bool process_batch(DBWrapper& db, const std::vector<Transaction>& transactions) {
    bool success = db.bulk_insert_transactions(transactions);
    if (success) {
        std::cout << "Inserted " << transactions.size() << " transactions" << std::endl;
    } else {
        std::cerr << "Failed to insert batch of " << transactions.size() << " transactions" << std::endl;
    }
    return success;
}

void query_date_range(DBWrapper& db, int64_t start_timestamp, int64_t end_timestamp) {
    auto range_txs = db.get_transactions_by_date_range(start_timestamp, end_timestamp);
    std::cout << "Found " << range_txs.size() << " transactions in specified date range" << std::endl;
    for (const auto& tx : range_txs) {
        std::cout << "Transaction: " << tx.id() 
                  << ", Amount: " << tx.amount_smallest_unit() 
                  << " " << tx.currency() << std::endl;
    }
}

int main() {
    try {
        // Initialize the database
        DBWrapper db("./stickylogs_db");

        // Insert some sample transactions
        std::vector<Transaction> transactions;
        transactions.emplace_back("TX004", "REF001", "USD", 105000, get_current_timestamp());
        transactions.emplace_back("TX005", "REF002", "KES", 150000, get_current_timestamp() - 86400);
        transactions.emplace_back("TX006", "REF003", "NGN", 250000, get_current_timestamp() - 172800);

        std::cout << "Inserting sample transactions..." << std::endl;
        for (const auto& tx : transactions) {
            if (db.insert_transaction(tx)) {
                std::cout << "Inserted transaction successfully: " << tx.id() << std::endl;
            } else {
                std::cout << "Failed to insert transaction: " << tx.id() << std::endl;
            }
        }

        // Retrieve a specific transaction
        std::cout << "\nRetrieving transaction TX002..." << std::endl;
        auto tx = db.get_transaction("TX002");
        if (tx) {
            std::cout << "Successfully retrieved transaction: " << tx->id() 
                      << ", Amount: " << tx->amount_smallest_unit() 
                      << " " << tx->currency() << std::endl;
        } else {
            std::cout << "Transaction not found." << std::endl;
        }

        // date range query
        std::cout << "\nRetrieving transactions from the last 2 days..." << std::endl;
        auto range_txs = db.get_transactions_by_date_range(
            get_current_timestamp() - 172800,  // 2 days ago
            get_current_timestamp()            // now
        );

        std::cout << "Found " << range_txs.size() << " transactions:" << std::endl;
        for (const auto& tx : range_txs) {
            std::cout << "Transaction: " << tx.id() 
                      << ", Amount: " << tx.amount_smallest_unit() 
                      << " " << tx.currency() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}