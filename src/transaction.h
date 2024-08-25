#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

class Transaction {
public:
    Transaction(std::string id, std::string reference, std::string currency, int64_t amount_smallest_unit, int64_t timestamp);

    // Disable copy constructor and assignment operator
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    // Enable move constructor and assignment operator
    Transaction(Transaction&&) noexcept = default;
    Transaction& operator=(Transaction&&) noexcept = default;

    const std::string& id() const { return id_; }
    const std::string& reference() const { return reference_; }
    const std::string& currency() const { return currency_; }
    int64_t amount_smallest_unit() const { return amount_smallest_unit_; }
    int64_t timestamp() const { return timestamp_; }
    std::vector<char> serialize() const;
    static Transaction deserialize(const char* data, size_t size);
    bool is_valid() const;

private:
    std::string id_;
    std::string reference_;
    std::string currency_;
    int64_t amount_smallest_unit_;
    int64_t timestamp_;
};