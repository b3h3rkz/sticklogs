#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "transaction.h"
#include "db_wrapper.h"
#include <iostream>
#include <vector>
#include <string>
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <db_path>" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];
    try {
        DBWrapper db(db_path);

        std::string command;
        while (std::cout << "> " && std::cin >> command) {
            if (command == "exit") {
                break;
            } else if (command == "insert") {
                // data format: insert TX001 REF001 USD 10000 1628097600
                std::string id, ref, currency;
                int64_t amount, timestamp;
                if (std::cin >> id >> ref >> currency >> amount >> timestamp) {
                    Transaction tx(id, ref, currency, amount, timestamp);
                    process_transaction(db, tx);
                }
            } else if (command == "query") {
                // sample format for this: query 1628097600 1628184000
                int64_t start, end;
                if (std::cin >> start >> end) {
                    query_date_range(db, start, end);
                }
            } else {
                std::cout << "Unknown command. Available commands: insert, query, exit" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}