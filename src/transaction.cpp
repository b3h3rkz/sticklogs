#include "transaction.h"
#include <cstring>
#include <stdexcept>

Transaction::Transaction(std::string_view id, std::string_view reference, 
                         std::string_view currency, int64_t amount_smallest_unit,
                         int64_t timestamp)
    : id_(id), reference_(reference), 
      currency_(currency), amount_smallest_unit_(amount_smallest_unit),
      timestamp_(timestamp) {}

Transaction::Transaction(Transaction&& other) noexcept
    : id_(std::move(other.id_)),
      reference_(std::move(other.reference_)),
      currency_(std::move(other.currency_)),
      amount_smallest_unit_(other.amount_smallest_unit_),
      timestamp_(other.timestamp_) {}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        id_ = std::move(other.id_);
        reference_ = std::move(other.reference_);
        currency_ = std::move(other.currency_);
        amount_smallest_unit_ = other.amount_smallest_unit_;
        timestamp_ = other.timestamp_;
    }
    return *this;
}

std::vector<char> Transaction::serialize() const {
    std::vector<char> buffer;
    buffer.reserve(id_.size() + reference_.size() + currency_.size() + 
                   2 * sizeof(int64_t) + 3 * sizeof(uint32_t));

    auto append = [&buffer](const std::string& s) {
        uint32_t size = s.size();
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&size), reinterpret_cast<const char*>(&size) + sizeof(size));
        buffer.insert(buffer.end(), s.begin(), s.end());
    };

    append(id_);
    append(reference_);
    append(currency_);
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&amount_smallest_unit_), 
                  reinterpret_cast<const char*>(&amount_smallest_unit_) + sizeof(amount_smallest_unit_));
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&timestamp_), 
                  reinterpret_cast<const char*>(&timestamp_) + sizeof(timestamp_));

    return buffer;
}

Transaction Transaction::deserialize(const char* data, size_t size) {
    const char* end = data + size;
    auto read_string = [&data, end]() -> std::string {
        if (data + sizeof(uint32_t) > end) throw std::runtime_error("Incomplete data");
        uint32_t size;
        std::memcpy(&size, data, sizeof(size));
        data += sizeof(size);
        if (data + size > end) throw std::runtime_error("Incomplete data");
        std::string result(data, data + size);
        data += size;
        return result;
    };

    std::string id = read_string();
    std::string reference = read_string();
    std::string currency = read_string();
    if (data + sizeof(int64_t) * 2 > end) throw std::runtime_error("Incomplete data");
    int64_t amount_smallest_unit;
    std::memcpy(&amount_smallest_unit, data, sizeof(amount_smallest_unit));
    data += sizeof(amount_smallest_unit);
    int64_t timestamp;
    std::memcpy(&timestamp, data, sizeof(timestamp));

    return Transaction(id, reference, currency, amount_smallest_unit, timestamp);
}

bool Transaction::is_valid() const {
    return !id_.empty() && !reference_.empty() && !currency_.empty() && timestamp_ > 0;
}