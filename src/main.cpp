#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "transaction.h"
#include "db_wrapper.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <csignal>
#include <sstream>


using json = nlohmann::json;
using boost::asio::ip::tcp;

// DBWrapper* g_db = nullptr;

std::atomic<bool> g_running(true);
boost::asio::io_context* g_io_context = nullptr;

void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ". Shutting down..." << std::endl;
    g_running = false;

    if (g_io_context) {
        g_io_context->stop();
    }
}

void handle_connection(tcp::socket socket, DBWrapper& db) {
    try {
        boost::asio::streambuf request;
        boost::asio::read_until(socket, request, "\r\n\r\n");

        std::string header = boost::asio::buffer_cast<const char*>(request.data());
        std::cout << "Received header:\n" << header << std::endl;

        std::istringstream header_stream(header);
        std::string line;
        int content_length = 0;
        while (std::getline(header_stream, line) && line != "\r") {
            if (line.substr(0, 16) == "Content-Length: ") {
                content_length = std::stoi(line.substr(16));
                break;
            }
        }

        std::vector<char> body(content_length);
        boost::asio::read(socket, boost::asio::buffer(body));

        std::string body_str(body.begin(), body.end());
        std::cout << "Received body:\n" << body_str << std::endl;

        json j = json::parse(body_str);
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
        
    // Prepare and send the HTTP response
    std::string response_body = response.dump();
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\n";
    response_stream << "Content-Type: application/json\r\n";
    response_stream << "Content-Length: " << response_body.length() << "\r\n";
    response_stream << "Connection: close\r\n\r\n";
    response_stream << response_body;    

    std::string response_str = response_stream.str();
    boost::asio::write(socket, boost::asio::buffer(response_str));
    std::cout << "Response sent:\n" << response_str << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
        std::string error_message = std::string("Error: ") + e.what();
        std::string error_response = "HTTP/1.1 500 Internal Server Error\r\n"
                                     "Content-Type: application/json\r\n"
                                     "Content-Length: " + std::to_string(error_message.length()) + "\r\n"
                                     "Connection: close\r\n\r\n" + error_message;
        boost::asio::write(socket, boost::asio::buffer(error_response));
        std::cout << "Sent error response:\n" << error_response << std::endl;
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
    const unsigned short port = 54321;

    //signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // g_db = new DBWrapper(db_path);
    try {
        std::cout << "=== StickyLogs Service ===" << std::endl;
        std::cout << "Starting service at: " << get_current_time() << std::endl;
        
        std::filesystem::path abs_path = std::filesystem::absolute(db_path);
        std::cout << "Database path: " << abs_path << std::endl;

        DBWrapper db(db_path);
        std::cout << "Database opened successfully." << std::endl;

        boost::asio::io_context io_context;         
        g_io_context = &io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));

        std::cout << "Listening on port " << port << std::endl;
        std::cout << "Service is ready to accept connections." << std::endl;
        std::cout << "Press Ctrl+C to stop the service." << std::endl;

        std::thread io_thread([&io_context]() {
            while (g_running) {
                io_context.run_for(std::chrono::seconds(1));
                io_context.restart();
            }
        });

        while (g_running) {
            boost::system::error_code ec;
            tcp::socket socket(io_context);
            
            acceptor.accept(socket, ec);
            
            if (!ec) {
                std::cout << "New connection accepted at: " << get_current_time() << std::endl;
                std::thread(handle_connection, std::move(socket), std::ref(db)).detach();
            }
        }
        io_thread.join();
        std::cout << "Shutting down service at: " << get_current_time() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}