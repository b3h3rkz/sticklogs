#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "db_wrapper.h"
#include "transaction.h"

using json = nlohmann::json;
using boost::asio::ip::tcp;

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

void handle_connection(tcp::socket& socket, DBWrapper& db) {
    std::cout << "New connection handled at: " << get_current_time() << std::endl;
    try {
        if (!socket.is_open()) {
            std::cerr << "Socket is not open" << std::endl;
            return;
        }

        boost::asio::streambuf request;
        boost::system::error_code ec;

        // Read the request
        size_t bytes_transferred = boost::asio::read_until(socket, request, "\r\n\r\n", ec);
        if (ec) {
            std::cerr << "Error reading request: " << ec.message() << std::endl;
            std::cerr << "Bytes transferred before error: " << bytes_transferred << std::endl;
            return;
        }
        std::cout << "Read " << bytes_transferred << " bytes from request" << std::endl;

        // Convert the request to a string
        std::string request_str(boost::asio::buffers_begin(request.data()),
                                boost::asio::buffers_begin(request.data()) + bytes_transferred);
        std::cout << "Request headers:\n" << request_str << std::endl;

        // Parse the request body
        std::istream request_stream(&request);
        std::string header;
        while (std::getline(request_stream, header) && header != "\r") {
            std::cout << "Header: " << header << std::endl;
        }

        // Read the request body
        size_t content_length = 0;
        if (request_str.find("Content-Length: ") != std::string::npos) {
            content_length = std::stoul(request_str.substr(request_str.find("Content-Length: ") + 16));
            std::cout << "Content-Length: " << content_length << std::endl;
        }

        std::string body;
        if (content_length > 0) {
            body.resize(content_length);
            request_stream.read(&body[0], content_length);
            std::cout << "Request body:\n" << body << std::endl;
        }

        // Parse JSON
        json j;
        try {
            j = json::parse(body);
            std::cout << "JSON parsed successfully" << std::endl;
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            // Send an error response
            json error_response = {
                {"success", false},
                {"message", "Invalid JSON in request body"}
            };
            std::string response_body = error_response.dump(4);
            std::string response_str = "HTTP/1.1 400 Bad Request\r\n";
            response_str += "Content-Type: application/json\r\n";
            response_str += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
            response_str += "\r\n";
            response_str += response_body;

            boost::system::error_code ec;
            boost::asio::write(socket, boost::asio::buffer(response_str), ec);
            if (ec) {
                std::cerr << "Error sending error response: " << ec.message() << std::endl;
            } else {
                std::cout << "Error response sent successfully" << std::endl;
            }
            return;
        }

        // Handle the request
        std::string action = j["action"];
        std::cout << "Action: " << action << std::endl;
        
        json response;
        
        if (action == "insert") {
            std::cout << "Processing insert action" << std::endl;
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
            response["transaction"] = {
                {"id", tx.id()},
                {"reference", tx.reference()},
                {"currency", tx.currency()},
                {"amount_smallest_unit", tx.amount_smallest_unit()},
                {"timestamp", tx.timestamp()}
            };
        }
        else if (action == "query") {
            std::cout << "Processing query action" << std::endl;
            int64_t start_timestamp = j["start_timestamp"].get<int64_t>();
            int64_t end_timestamp = j["end_timestamp"].get<int64_t>();

            std::vector<Transaction> transactions = db.get_transactions_by_date_range(start_timestamp, end_timestamp);

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
            response["message"] = "Query executed successfully";
        }
        else {
            std::cerr << "Unknown action: " << action << std::endl;
            response["success"] = false;
            response["message"] = "Unknown action";
        }

        // Generate and send the response
        std::string response_body = response.dump(4);  // Pretty print with 4-space indent
        std::string response_str = "HTTP/1.1 200 OK\r\n";
        response_str += "Content-Type: application/json\r\n";
        response_str += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
        response_str += "\r\n";
        response_str += response_body;

        std::cout << "Sending response:\n" << response_str << std::endl;

        boost::asio::write(socket, boost::asio::buffer(response_str), ec);
        if (ec) {
            std::cerr << "Error sending response: " << ec.message() << std::endl;
        } else {
            std::cout << "Response sent successfully" << std::endl;
        }

        // Close the socket
        socket.shutdown(tcp::socket::shutdown_both, ec);
        if (ec) std::cerr << "Error shutting down socket: " << ec.message() << std::endl;
        socket.close(ec);
        if (ec) std::cerr << "Error closing socket: " << ec.message() << std::endl;

        std::cout << "Connection closed" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in handle_connection: " << e.what() << std::endl;
    }
    std::cout << "Connection handling completed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <db_path>" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];
    const unsigned short port = 54321;

    try {
        std::cout << "=== StickyLogs Service ===" << std::endl;
        std::cout << "Starting service at: " << get_current_time() << std::endl;
        
        DBWrapper db(db_path);
        std::cout << "Database opened successfully." << std::endl;

        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));

        std::cout << "Listening on port " << port << std::endl;
        std::cout << "Service is ready to accept connections." << std::endl;

        while (true) {
            std::cout << "Waiting for connection..." << std::endl;
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::cout << "New connection accepted at: " << get_current_time() << std::endl;
            
            // Handle the connection in the main thread
            handle_connection(socket, db);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}