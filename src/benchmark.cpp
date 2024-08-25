#include "log.h"
#include "db_wrapper.h"
#include <chrono>
#include <random>
#include <iostream>
#include <vector>

class LogGenerator {
private:
    std::mt19937 gen;
    std::uniform_int_distribution<> event_dist;
    int id_counter;

public:
    LogGenerator() : 
        gen(std::chrono::system_clock::now().time_since_epoch().count()),
        event_dist(0, 4),
        id_counter(0) {}

    Log generate() {
        std::string reference = "REF" + std::to_string(++id_counter);
        nlohmann::json metadata;
        metadata["event"] = "Event" + std::to_string(event_dist(gen));
        metadata["details"] = "This is log entry " + std::to_string(id_counter);
        
        return Log(reference, metadata);
    }
};

template<typename Func>
double benchmark(Func f, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return diff.count() / iterations;
}

void run_benchmarks() {
    try {
        DBWrapper db("./benchmark_db");
        LogGenerator gen;

        // Benchmark for single insert
        double single_insert_time = benchmark([&]() {
            db.insert_log(gen.generate());
        }, 1000);
        std::cout << "Average time for single insert: " << single_insert_time << " seconds\n";

        // Benchmark for multiple inserts
        double multiple_insert_time = benchmark([&]() {
            for (int i = 0; i < 100; ++i) {
                db.insert_log(gen.generate());
            }
        }, 10);
        std::cout << "Average time for inserting 100 logs: " << multiple_insert_time << " seconds\n";

        // Benchmark for retrieval
        double retrieval_time = benchmark([&]() {
            db.get_log("REF1");
        }, 1000);
        std::cout << "Average time for single retrieval: " << retrieval_time << " seconds\n";

        // Benchmark for range query
        int64_t end_time = std::chrono::system_clock::now().time_since_epoch().count();
        int64_t start_time = end_time - 86400000; // Last 24 hours in milliseconds
        double range_query_time = benchmark([&]() {
            db.get_logs_by_time_range(start_time, end_time);
        }, 100);
        std::cout << "Average time for fetching all logs added in the last 24hrs: " << range_query_time << " seconds\n";

    } catch (const std::exception& e) {
        std::cerr << "Error in benchmarking: " << e.what() << std::endl;
    }
}

