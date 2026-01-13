#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include <string>
#include <string_view>

namespace Address {

constexpr size_t BYTES = 20;
constexpr size_t HEX_LEN = 40;
constexpr size_t STR_LEN = 42;  // 0x + 40 hex

/**
 * Check if string is valid EVM address format
 */
bool is_valid(std::string_view address);

/**
 * Check if address is the zero address
 */
bool is_zero(std::string_view address);

/**
 * Verify EIP-55 checksum
 */
bool verify_checksum(std::string_view address);

/**
 * Convert address to EIP-55 checksummed format
 */
std::string to_checksum(std::string_view address);

/**
 * Convert address to lowercase
 */
std::string to_lower(std::string_view address);

}

#endif /* ADDRESS_HPP */
