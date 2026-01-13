#include "address.hpp"

extern "C" {
#include "../hex/hex.h"
#include "../sha3/sha3.h"
}

#include <algorithm>
#include <cctype>

namespace Address {

bool is_valid(std::string_view address) {
    if (address.size() != STR_LEN) {
        return false;
    }
    
    if (!has_hex_prefix(address.data())) {
        return false;
    }
    
    return is_hex_string(address.data() + 2, HEX_LEN);
}

bool is_zero(std::string_view address) {
    if (!is_valid(address)) {
        return false;
    }
    
    for (size_t i = 2; i < STR_LEN; i++) {
        if (address[i] != '0') {
            return false;
        }
    }
    return true;
}

std::string to_lower(std::string_view address) {
    std::string result = "0x";
    result.reserve(STR_LEN);
    
    for (size_t i = 2; i < address.size(); i++) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(address[i])));
    }
    return result;
}

std::string to_checksum(std::string_view address) {
    if (!is_valid(address)) {
        return "";
    }
    
    // Get lowercase (without 0x)
    std::string lower;
    lower.reserve(HEX_LEN);
    for (size_t i = 2; i < address.size(); i++) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(address[i])));
    }
    
    // Hash the lowercase address
    SHA3_256_CTX ctx;
    uint8_t hash[32];
    
    KECCAK_256_Init(&ctx);
    KECCAK_256_Update(&ctx, reinterpret_cast<const uint8_t*>(lower.data()), HEX_LEN);
    KECCAK_256_Final(hash, &ctx);
    
    // Build checksummed address
    std::string result = "0x";
    result.reserve(STR_LEN);
    
    for (size_t i = 0; i < HEX_LEN; i++) {
        char c = lower[i];
        uint8_t hash_byte = hash[i / 2];
        uint8_t nibble = (i % 2 == 0) ? (hash_byte >> 4) : (hash_byte & 0x0F);
        
        if (nibble >= 8 && c >= 'a' && c <= 'f') {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        } else {
            result += c;
        }
    }
    
    return result;
}

bool verify_checksum(std::string_view address) {
    if (!is_valid(address)) {
        return false;
    }
    
    std::string expected = to_checksum(address);
    return address == expected;
}

}
