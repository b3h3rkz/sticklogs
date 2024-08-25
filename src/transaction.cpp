#include "transaction.h"
#include <cstring>
#include <stdexcept>

Transaction::Transaction(std::string id, std::string reference, std::string currency, int64_t amount_smallest_unit, int64_t timestamp)
    : m_id(std::move(id)), m_reference(std::move(reference)), m_currency(std::move(currency)), 
      m_amount_smallest_unit(amount_smallest_unit), m_timestamp(timestamp) {}

std::string Transaction::id() const { return m_id; }
std::string Transaction::reference() const { return m_reference; }
std::string Transaction::currency() const { return m_currency; }
int64_t Transaction::amount_smallest_unit() const { return m_amount_smallest_unit; }
int64_t Transaction::timestamp() const { return m_timestamp; }

std::vector<char> Transaction::serialize() const {
    std::vector<char> result;

    auto serialize_string = [&result](const std::string& str) {
        uint32_t size = str.size();
        result.insert(result.end(), reinterpret_cast<char*>(&size), reinterpret_cast<char*>(&size) + sizeof(size));
        result.insert(result.end(), str.begin(), str.end());
    };

    serialize_string(m_id);
    serialize_string(m_reference);
    serialize_string(m_currency);

    result.insert(result.end(), reinterpret_cast<const char*>(&m_amount_smallest_unit), reinterpret_cast<const char*>(&m_amount_smallest_unit) + sizeof(m_amount_smallest_unit));
    result.insert(result.end(), reinterpret_cast<const char*>(&m_timestamp), reinterpret_cast<const char*>(&m_timestamp) + sizeof(m_timestamp));

    return result;
}

Transaction Transaction::deserialize(const char* data, size_t size) {
    if (size < 12) {
        throw std::runtime_error("Invalid serialized data size");
    }

    const char* ptr = data;
    const char* end = data + size;

    auto deserialize_string = [&ptr, end]() {
        if (ptr + sizeof(uint32_t) > end) {
            throw std::runtime_error("Unexpected end of data");
        }
        uint32_t size;
        std::memcpy(&size, ptr, sizeof(size));
        ptr += sizeof(size);
        if (ptr + size > end) {
            throw std::runtime_error("Unexpected end of data");
        }
        std::string result(ptr, ptr + size);
        ptr += size;
        return result;
    };

    std::string id = deserialize_string();
    std::string reference = deserialize_string();
    std::string currency = deserialize_string();

    if (ptr + sizeof(int64_t) * 2 > end) {
        throw std::runtime_error("Unexpected end of data");
    }

    int64_t amount_smallest_unit;
    std::memcpy(&amount_smallest_unit, ptr, sizeof(amount_smallest_unit));
    ptr += sizeof(amount_smallest_unit);

    int64_t timestamp;
    std::memcpy(&timestamp, ptr, sizeof(timestamp));

    return Transaction(id, reference, currency, amount_smallest_unit, timestamp);
}