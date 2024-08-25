#include "transaction.h"
#include <cstring>
#include <stdexcept>

Transaction::Transaction(std::string id, std::string reference, std::string currency, int64_t amount_smallest_unit, int64_t timestamp)
    : id_(std::move(id)), reference_(std::move(reference)), currency_(std::move(currency)),
      amount_smallest_unit_(amount_smallest_unit), timestamp_(timestamp) {}

std::vector<char> Transaction::serialize() const {
    std::vector<char> buffer;
    
    // Serialize strings
    auto serialize_string = [&buffer](const std::string& str) {
        uint32_t size = str.size();
        buffer.insert(buffer.end(), reinterpret_cast<char*>(&size), reinterpret_cast<char*>(&size) + sizeof(size));
        buffer.insert(buffer.end(), str.begin(), str.end());
    };

    serialize_string(id_);
    serialize_string(reference_);
    serialize_string(currency_);

    // Serialize int64_t values
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&amount_smallest_unit_), reinterpret_cast<const char*>(&amount_smallest_unit_) + sizeof(amount_smallest_unit_));
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&timestamp_), reinterpret_cast<const char*>(&timestamp_) + sizeof(timestamp_));

    return buffer;
}

Transaction Transaction::deserialize(const char* data, size_t size) {
    const char* ptr = data;
    const char* end = data + size;

    auto deserialize_string = [&ptr, end]() {
        if (ptr + sizeof(uint32_t) > end) throw std::runtime_error("Incomplete data");
        uint32_t size;
        std::memcpy(&size, ptr, sizeof(size));
        ptr += sizeof(size);
        if (ptr + size > end) throw std::runtime_error("Incomplete data");
        std::string result(ptr, ptr + size);
        ptr += size;
        return result;
    };

    std::string id = deserialize_string();
    std::string reference = deserialize_string();
    std::string currency = deserialize_string();

    if (ptr + sizeof(int64_t) * 2 > end) throw std::runtime_error("Incomplete data");
    int64_t amount_smallest_unit, timestamp;
    std::memcpy(&amount_smallest_unit, ptr, sizeof(int64_t));
    ptr += sizeof(int64_t);
    std::memcpy(&timestamp, ptr, sizeof(int64_t));

    return Transaction(std::move(id), std::move(reference), std::move(currency), amount_smallest_unit, timestamp);
}

bool Transaction::is_valid() const {
    return !id_.empty() && !reference_.empty() && !currency_.empty() && timestamp_ > 0;
}