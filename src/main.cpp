#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "transaction.h"
#include "db_wrapper.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

using json = nlohmann::json;
using boost::asio::ip::tcp;

void handle_connection(tcp::socket socket, DBWrapper& db) {
    try {
        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, "\n");
        std::string data = boost::asio::buffer_cast<const char*>(buf.data());
        
        json j = json::parse(data);
        std::string action = j["action"];
        
        json response;
        
        if (action == "insert") {
            Transaction tx(j["id"], j["reference"], j["currency"], 
                           j["amount_smallest_unit"], j["timestamp"]);
            bool success = db.insert_transaction(tx);
            response["success"] = success;
            response["message"] = success ? "Transaction saved successfully" : "Failed to save transaction";
        }
        else if (action == "batch_insert") {
            std::vector<Transaction> transactions;
            for (const auto& tx_json : j["transactions"]) {
                transactions.emplace_back(tx_json["id"], tx_json["reference"], tx_json["currency"],
                                          tx_json["amount_smallest_unit"], tx_json["timestamp"]);
            }
            bool success = db.bulk_insert_transactions(transactions);
            response["success"] = success;
            response["message"] = success ? "Batch saved successfully" : "Failed to save batch";
        }
        else if (action == "query") {
            int64_t start_timestamp = j["start_timestamp"];
            int64_t end_timestamp = j["end_timestamp"];
            auto transactions = db.get_transactions_by_date_range(start_timestamp, end_timestamp);
            response["success"] = true;
            response["transactions"] = json::array();
            for (const auto& tx : transactions) {
                response["transactions"].push_back({
                    {"id", tx.id()},
                    {"reference", tx.reference()},
                    {"currency", tx.currency()},
                    {"amount_smallest_unit", tx.amount_smallest_unit()},
                    {"timestamp", tx.timestamp()}
                });
            }
        }
        else {
            response["success"] = false;
            response["message"] = "Unknown endpoint or action";
        }
        
        std::string response_str = response.dump() + "\n";
        boost::asio::write(socket, boost::asio::buffer(response_str));
    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
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