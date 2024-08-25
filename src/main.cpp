#include <iostream>
#include <string>
#include <fstream> 
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
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

class ThreadSafeQueue {
private:
    std::queue<std::function<void()>> queue;
    std::mutex mutex;
    std::condition_variable cond;

public:
    void push(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push(std::move(task));
        cond.notify_one();
    }

    std::function<void()> pop() {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this] { return !queue.empty(); });
        auto task = std::move(queue.front());
        queue.pop();
        return task;
    }
};

class ThreadPool {
private:
    std::vector<std::thread> workers;
    ThreadSafeQueue tasks;
    std::atomic<bool> stop;

public:
    ThreadPool(size_t num_threads) : stop(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (!stop) {
                    auto task = tasks.pop();
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        stop = true;
        for (size_t i = 0; i < workers.size(); ++i) {
            tasks.push([] {});
        }
        for (auto& worker : workers) {
            worker.join();
        }
    }

    void enqueue(std::function<void()> task) {
        tasks.push(std::move(task));
    }
};

void handle_connection(tcp::socket& socket, DBWrapper& db) {
    std::cout << "New connection handled at: " << get_current_time() << std::endl;
    try {
        boost::asio::streambuf request;
        boost::system::error_code ec;

        size_t bytes_transferred = boost::asio::read_until(socket, request, "\r\n\r\n", ec);
        if (ec) {
            std::cerr << "Error reading request: " << ec.message() << std::endl;
            return;
        }

        std::string request_str(boost::asio::buffers_begin(request.data()),
                                boost::asio::buffers_begin(request.data()) + bytes_transferred);
        
        size_t content_length = 0;
        if (request_str.find("Content-Length: ") != std::string::npos) {
            content_length = std::stoul(request_str.substr(request_str.find("Content-Length: ") + 16));
        }

        std::string body;
        if (content_length > 0) {
            body.resize(content_length);
            boost::asio::read(socket, boost::asio::buffer(body), ec);
            if (ec) {
                std::cerr << "Error reading body: " << ec.message() << std::endl;
                return;
            }
        }

        json j = json::parse(body);
        std::string action = j["action"];
        
        json response;
        
        if (action == "insert") {
            Log log(j["reference"], j["metadata"]);
            bool success = db.insert_log(log);
            response["success"] = success;
            response["message"] = success ? "Log saved successfully" : "Failed to save log";
            response["log"] = {
                {"reference", log.reference()},
                {"metadata", log.metadata()},
                {"timestamp", log.timestamp()}
            };
        }
        else if (action == "query_by_reference") {
            std::string reference = j["reference"];
            try {
                Log log = db.get_log(reference);
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
            std::vector<Log> logs = db.get_logs_by_time_range(start_timestamp, end_timestamp);
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
        else {
            response["success"] = false;
            response["message"] = "Unknown action";
        }

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
        DBWrapper db(db_path, CONFIG_PATH);

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

        ThreadPool pool(std::thread::hardware_concurrency());

        std::vector<std::shared_ptr<tcp::socket>> sockets(max_connections);
        for (int i = 0; i < max_connections; ++i) {
            sockets[i] = std::make_shared<tcp::socket>(io_context);
        }

        std::function<void(std::shared_ptr<tcp::socket>, size_t)> accept_connection;
        accept_connection = [&](std::shared_ptr<tcp::socket> socket, size_t index) {
            acceptor.async_accept(*socket, [&, socket, index](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "New connection accepted at: " << get_current_time() << std::endl;
                    pool.enqueue([socket, &db] {
                        handle_connection(*socket, db);
                    });
                    accept_connection(sockets[index], index);
                } else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }
            });
        };

        for (size_t i = 0; i < max_connections; ++i) {
            accept_connection(sockets[i], i);
        }

        io_context.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}