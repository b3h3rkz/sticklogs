#include "transaction.h"
#include "db_wrapper.h"
#include <chrono>
#include <random>
#include <iostream>
#include <vector>

class TransactionGenerator {
private:
    std::mt19937 gen;
    std::uniform_int_distribution<> amount_dist;
    std::uniform_int_distribution<> currency_dist;
    const std::vector<std::string> currencies = {"USD", "EUR", "GHS", "KES", "NGN"};
    int id_counter;

public:
    TransactionGenerator() : 
        gen(std::chrono::system_clock::now().time_since_epoch().count()),
        amount_dist(1, 1000),
        currency_dist(0, 4),
        id_counter(0) {}

    Transaction generate() {
        std::string id = "TX" + std::to_string(++id_counter);
        std::string reference = "REF" + std::to_string(id_counter);
        int64_t amount = amount_dist(gen);
        std::string currency = currencies[currency_dist(gen)];
        int64_t timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        
        return Transaction(id, reference, currency, amount, timestamp);
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
        TransactionGenerator gen;

        // for benchmarking a single insert benchmark
        double single_insert_time = benchmark([&]() {
            db.insert_transaction(gen.generate());
        }, 1000);
        std::cout << "Average time for single insert: " << single_insert_time << " seconds\n";

        // for bulk insert benchmark
        double bulk_insert_time = benchmark([&]() {
            std::vector<Transaction> transactions;
            for (int i = 0; i < 1000; ++i) {
                transactions.push_back(gen.generate());
            }
            db.bulk_insert_transactions(transactions);
        }, 10);
        std::cout << "Average time for bulk insert of 1,000 transactions: " << bulk_insert_time << " seconds\n";

        // Retrieval benchmark
        double retrieval_time = benchmark([&]() {
            db.get_transaction("TX1");
        }, 1000);
        std::cout << "Average time for single retrieval: " << retrieval_time << " seconds\n";

        // Range query benchmark
        int64_t end_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        int64_t start_time = end_time - 86400; // Last 24 hours
        double range_query_time = benchmark([&]() {
            db.get_transactions_by_date_range(start_time, end_time);
        }, 100);
        std::cout << "Average time fetching all data added in the last 24hrs: " << range_query_time << " seconds\n";

    } catch (const std::exception& e) {
        std::cerr << "Error in benchmarking: " << e.what() << std::endl;
    }
}

