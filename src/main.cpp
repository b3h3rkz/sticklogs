#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "db_wrapper.h"
#include "log.h"
#include <boost/filesystem.hpp>

using json = nlohmann::json;
using boost::asio::ip::tcp;

namespace fs = boost::filesystem;

const std::string CONFIG_PATH = "../config/db_config.yaml";

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

void handle_connection(boost::asio::io_context& io_context, tcp::socket socket, std::shared_ptr<DBWrapper> db) {
    std::cout << "New connection handled at: " << get_current_time() << std::endl;
    try {
        boost::asio::streambuf request;
        boost::system::error_code ec;

        std::cout << "Reading request..." << std::endl;  // Debug log
        size_t bytes_transferred = boost::asio::read_until(socket, request, "\r\n\r\n", ec);
        if (ec) {
            std::cerr << "Error reading request: " << ec.message() << std::endl;
            return;
        }
        std::cout << "Request read successfully. Bytes transferred: " << bytes_transferred << std::endl;  // Debug log

        std::string request_str(boost::asio::buffers_begin(request.data()),
                                boost::asio::buffers_begin(request.data()) + bytes_transferred);
        
        std::cout << "Request headers:\n" << request_str << std::endl;  // Debug log

        size_t content_length = 0;
        if (request_str.find("Content-Length: ") != std::string::npos) {
            content_length = std::stoul(request_str.substr(request_str.find("Content-Length: ") + 16));
            std::cout << "Content-Length: " << content_length << std::endl;  // Debug log
        }

        std::string body;
        if (content_length > 0) {
            std::cout << "Reading body..." << std::endl;  // Debug log
            body.resize(content_length);
            
            boost::asio::steady_timer timer(io_context);
            timer.expires_after(std::chrono::seconds(5));  // 5-second timeout

            boost::system::error_code ec;
            auto read_handler = [&](const boost::system::error_code& error, std::size_t bytes_transferred) {
                timer.cancel();
                ec = error;
            };

            auto timeout_handler = [&](const boost::system::error_code& error) {
                if (!error) {
                    socket.cancel();
                    ec = boost::asio::error::timed_out;
                }
            };

            timer.async_wait(timeout_handler);
            boost::asio::async_read(socket, boost::asio::buffer(body), read_handler);
            
            io_context.run();  // Run the io_context to process the asynchronous operations

            if (ec) {
                if (ec == boost::asio::error::timed_out) {
                    std::cerr << "Timeout reading body" << std::endl;
                } else {
                    std::cerr << "Error reading body: " << ec.message() << std::endl;
                }
                return;
            }
            std::cout << "Body read successfully. Length: " << body.length() << std::endl;  // Debug log
        }

        std::cout << "Received body: " << body << std::endl;  // Debug log

        json j = json::parse(body);
        std::string action = j["action"];
        
        std::cout << "Action: " << action << std::endl;  // Debug log
        
        json response;
        
        if (action == "insert") {
            Log log(j["reference"], j["metadata"]);
            bool success = db->insert_log(log);
            response["success"] = success;
            response["message"] = success ? "Log saved successfully" : "Failed to save log";
            response["log"] = {
                {"reference", log.reference()},
                {"metadata", log.metadata()},
                {"timestamp", log.timestamp()}
            };
        }
        else if (action == "batch_insert") {
            std::cout << "Handling batch insert" << std::endl;  // Debug log
            if (!j.contains("logs") || !j["logs"].is_array()) {
                std::cerr << "Invalid batch insert request: missing or invalid 'logs' field" << std::endl;
                throw std::runtime_error("Invalid batch insert request");
            }
            std::vector<Log> logs;
            for (const auto& log_json : j["logs"]) {
                if (!log_json.contains("reference") || !log_json.contains("metadata")) {
                    std::cerr << "Invalid log entry in batch: " << log_json.dump() << std::endl;
                    throw std::runtime_error("Invalid log entry in batch");
                }
                logs.emplace_back(log_json["reference"], log_json["metadata"]);
            }
            std::cout << "Parsed " << logs.size() << " logs" << std::endl;  // Debug log
            bool success = db->batch_insert_logs(logs);
            response["success"] = success;
            response["message"] = success ? "Logs saved successfully" : "Failed to save logs";
            response["count"] = logs.size();
            std::cout << "Batch insert completed. Success: " << success << std::endl;  // Debug log
        }
        else if (action == "query_by_reference") {
            std::string reference = j["reference"];
            try {
                Log log = db->get_log(reference);
                response["success"] = true;
                response["log"] = {
                    {"reference", log.reference()},
                    {"metadata", log.metadata()},
                    {"timestamp", log.timestamp()}
                };
                response["message"] = "Log found";
            } catch (const std::runtime_error& e) {
                response["success"] = false;
                response["message"] = "Log not found";
            }
        }
        else if (action == "query") {
            int64_t start_timestamp = j["start_timestamp"];
            int64_t end_timestamp = j["end_timestamp"];
            std::vector<Log> logs = db->get_logs_by_time_range(start_timestamp, end_timestamp);
            response["success"] = true;
            response["logs"] = json::array();
            for (const auto& log : logs) {
                response["logs"].push_back({
                    {"reference", log.reference()},
                    {"metadata", log.metadata()},
                    {"timestamp", log.timestamp()}
                });
            }
            response["message"] = "Query executed successfully";
        }
        else if (action == "query_all") {
            std::vector<Log> all_logs = db->get_all_logs();
            response["success"] = true;
            response["logs"] = json::array();
            for (const auto& log : all_logs) {
                response["logs"].push_back({
                    {"reference", log.reference()},
                    {"metadata", log.metadata()},
                    {"timestamp", log.timestamp()}
                });
            }
            response["message"] = "All logs retrieved successfully";
            response["count"] = all_logs.size();
        }
        else {
            response["success"] = false;
            response["message"] = "Unknown action";
        }

        std::cout << "Sending response: " << response.dump() << std::endl;  // Debug log

        std::string response_body = response.dump(4);
        std::string response_str = "HTTP/1.1 200 OK\r\n";
        response_str += "Content-Type: application/json\r\n";
        response_str += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
        response_str += "\r\n";
        response_str += response_body;

        boost::asio::write(socket, boost::asio::buffer(response_str), ec);
        if (ec) {
            std::cerr << "Error sending response: " << ec.message() << std::endl;
        }

        socket.shutdown(tcp::socket::shutdown_both, ec);
        socket.close(ec);
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in handle_connection: " << e.what() << std::endl;
        json error_response = {
            {"success", false},
            {"message", std::string("Error: ") + e.what()}
        };
        std::string error_body = error_response.dump();
        std::string error_response_str = "HTTP/1.1 400 Bad Request\r\n";
        error_response_str += "Content-Type: application/json\r\n";
        error_response_str += "Content-Length: " + std::to_string(error_body.length()) + "\r\n";
        error_response_str += "\r\n";
        error_response_str += error_body;
        boost::system::error_code ec;
        boost::asio::write(socket, boost::asio::buffer(error_response_str), ec);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <db_path>" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];

    // Check if config file exists and is readable
    std::ifstream config_file(CONFIG_PATH);
    if (!config_file.good()) {
        std::cerr << "Error: Unable to read config file at " << CONFIG_PATH << std::endl;
        return 1;
    }
    config_file.close();

    try {
        auto db = std::make_shared<DBWrapper>(db_path, CONFIG_PATH);

        const unsigned short port = 54321;
        const int max_connections = 10000;

        std::cout << "=== StickyLogs Service ===" << std::endl;
        std::cout << "Starting service at: " << get_current_time() << std::endl;
        std::cout << "Database path: " << db_path << std::endl;
        std::cout << "Config file: " << CONFIG_PATH << std::endl;
        std::cout << "Listening on port " << port << std::endl;
        std::cout << "Max connections: " << max_connections << std::endl;

        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));

        while (true) {
            tcp::socket socket(io_context);
            boost::system::error_code ec;
            acceptor.accept(socket, ec);

            if (ec) {
                std::cerr << "Accept error: " << ec.message() << std::endl;
                continue;  // Continue to the next iteration to accept new connections
            }

            std::cout << "New connection accepted at: " << get_current_time() << std::endl;

            std::thread([&io_context, socket = std::move(socket), db]() mutable {
                handle_connection(io_context, std::move(socket), db);
            }).detach();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}