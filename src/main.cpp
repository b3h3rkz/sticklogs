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
// #include <boost/asio/deadline_timer.hpp>


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
        std::cout << "New connection established." << std::endl;

        boost::asio::streambuf request;
        boost::system::error_code ec;


        // Read the headers
        boost::asio::read_until(socket, request, "\r\n\r\n", ec);
        if (ec) {
            throw boost::system::system_error(ec, "Failed to read headers");
        }

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
        if (content_length == 0) {
            throw std::runtime_error("Content-Length not found or zero");
        }


        // Read the JSON body in chunks
        std::string body;
        std::vector<char> chunk(1024);  // Read in 1KB chunks
        size_t bytes_read = 0;
        while (bytes_read < static_cast<size_t>(content_length)) {
            size_t chunk_size = std::min(static_cast<size_t>(content_length) - bytes_read, chunk.size());
            size_t n = boost::asio::read(socket, boost::asio::buffer(chunk, chunk_size), ec);
            if (ec && ec != boost::asio::error::eof) {
                throw boost::system::system_error(ec, "Failed to read body chunk");
            }
            body.append(chunk.begin(), chunk.begin() + n);
            bytes_read += n;
            if (ec == boost::asio::error::eof) {
                break;
            }
        }

        std::cout << "Received body:\n" << body << std::endl;

        json j = json::parse(body);
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

    std::string response_body = response.dump();
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\n";
    response_stream << "Content-Type: application/json\r\n";
    response_stream << "Content-Length: " << response_body.length() << "\r\n";
    response_stream << "Connection: close\r\n\r\n";
    response_stream << response_body;    

    std::string response_str = response_stream.str();
    boost::asio::write(socket, boost::asio::buffer(response_str), ec);
    if (ec) {
            throw boost::system::system_error(ec, "Failed to send response");
        }    
    std::cout << "Response sent:\n" << response_str << std::endl;
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
    catch (const std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
        std::string error_message = json({
            {"error", e.what()},
            {"type", typeid(e).name()}
        }).dump();
        std::string error_response = "HTTP/1.1 500 Internal Server Error\r\n"
                                    "Content-Type: application/json\r\n"
                                    "Content-Length: " + std::to_string(error_message.length()) + "\r\n"
                                    "Connection: close\r\n\r\n" + error_message;
        boost::system::error_code ec;
        boost::asio::write(socket, boost::asio::buffer(error_response), ec);
        if (ec) {
            std::cerr << "Failed to send error response: " << ec.message() << std::endl;
        } else {
            std::cout << "Sent error response:\n" << error_response << std::endl;
        }
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

    // Use sigaction instead of signal for more reliable signal handling
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

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

        while (g_running) {
            // boost::system::error_code ec;
            tcp::socket socket(io_context);
            
            // Use async_accept with a timeout to allow periodic checking of g_running
            boost::asio::steady_timer timer(io_context);
            acceptor.async_accept(socket, [&](const boost::system::error_code& error) {
                timer.cancel();
                if (!error && g_running) {
                    std::cout << "New connection accepted at: " << get_current_time() << std::endl;
                    std::thread(handle_connection, std::move(socket), std::ref(db)).detach();
                }
            });

            timer.expires_after(std::chrono::seconds(1));
            timer.async_wait([&](const boost::system::error_code& error) {
                if (!error) {
                    acceptor.cancel();
                }
            });

            io_context.run();
            io_context.restart();
        }

        std::cout << "Shutting down service at: " << get_current_time() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}