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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <db_path>" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];

    try {
        DBWrapper db(db_path);
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::thread(handle_connection, std::move(socket), std::ref(db)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}