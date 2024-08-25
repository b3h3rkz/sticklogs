#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

class Transaction {
public:
    Transaction(std::string id, std::string reference, std::string currency, int64_t amount_smallest_unit, int64_t timestamp);

    std::string id() const;
    std::string reference() const;
    std::string currency() const;
    int64_t amount_smallest_unit() const;
    int64_t timestamp() const;

    std::vector<char> serialize() const;
    static Transaction deserialize(const char* data, size_t size);

private:
    std::string m_id;
    std::string m_reference;
    std::string m_currency;
    int64_t m_amount_smallest_unit;
    int64_t m_timestamp;
};

#endif // TRANSACTION_H