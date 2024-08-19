#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "transaction.h"
#include "db_wrapper.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <csignal>

using json = nlohmann::json;
using boost::asio::ip::tcp;

DBWrapper* g_db = nullptr;

volatile sig_atomic_t g_running = 1;

void signal_handler(int signal) {
    g_running = 0;
}

void handle_connection(tcp::socket socket, DBWrapper& db) {
    try {
        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, "\n");
        std::string data = boost::asio::buffer_cast<const char*>(buf.data());
        std::cout << "Received data: " << data << std::endl;  // Log raw data

        json j = json::parse(data);
        std::string action = j["action"];
        
        json response;
        
    if (action == "insert") {
       Transaction tx(
        j["id"].get<std::string>(),
        j["reference"].get<std::string>(),
        j["currency"].get<std::string>(),
        j["amount_smallest_unit"].get<int64_t>(),
        j["timestamp"].get<int64_t>()
    );
    bool success = db.insert_transaction(tx);

    response["success"] = success;
    response["message"] = success ? "Transaction saved successfully" : "Failed to save transaction";
    }
    else if (action == "batch_insert") {
        std::vector<Transaction> transactions;
        for (const auto& tx_json : j["transactions"]) {
        transactions.emplace_back(
            tx_json["id"].get<std::string>(),
            tx_json["reference"].get<std::string>(),
            tx_json["currency"].get<std::string>(),
            tx_json["amount_smallest_unit"].get<int64_t>(),
            tx_json["timestamp"].get<int64_t>()
        );
    }
    bool success = db.bulk_insert_transactions(transactions);
    response["success"] = success;
    response["message"] = success ? "Batch saved successfully" : "Failed to save batch";
    }
    else if (action == "query") {
        int64_t start_timestamp = j["start_timestamp"].get<int64_t>();
        int64_t end_timestamp = j["end_timestamp"].get<int64_t>();
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

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <db_path>" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    g_db = new DBWrapper(db_path);
    try {
        std::cout << "=== Welcome To StickyLogs Service ===" << std::endl;
        std::cout << "Starting the service at: " << get_current_time() << std::endl;
        std::cout << "Database path: " << db_path << std::endl;

        DBWrapper db(db_path);
        std::cout << "Database opened successfully." << std::endl;

        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 65432));

        std::cout << "Listening on port 65432" << std::endl;
        std::cout << "Service is ready to accept connections." << std::endl;
        std::cout << "Press Ctrl+C to stop the service." << std::endl;

        while (g_running) {
            boost::system::error_code ec;
            tcp::socket socket(io_context);
            
            // Use accept with a timeout to allow checking g_running
            acceptor.accept(socket, ec);
            
            if (!ec) {
                std::cout << "New connection accepted at: " << get_current_time() << std::endl;
                std::thread(handle_connection, std::move(socket), std::ref(db)).detach();
            }
        }
        std::cout << "Shutting down service at: " << get_current_time() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    delete g_db;
    return 0;
}