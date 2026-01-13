#ifndef HEX_H
#define HEX_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check if character is valid hex digit
 */
bool is_hex_char(char c);

/**
 * Check if string contains only hex characters
 */
bool is_hex_string(const char *str, size_t len);

/**
 * Check if string has 0x prefix
 */
bool has_hex_prefix(const char *str);

/**
 * Convert hex character to nibble value (0-15)
 */
uint8_t hex_char_to_nibble(char c);

/**
 * Convert hex string to bytes
 * @param hex Input hex string (with or without 0x prefix)
 * @param out Output buffer (must be at least len/2 bytes)
 * @param hex_len Length of hex string
 * @return Number of bytes written, or -1 on error
 */
int hex_to_bytes(const char *hex, uint8_t *out, size_t hex_len);

/**
 * Convert bytes to lowercase hex string
 * @param bytes Input bytes
 * @param len Number of bytes
 * @param out Output buffer (must be at least len*2+1 bytes)
 * @param prefix If true, prepend "0x"
 */
void bytes_to_hex(const uint8_t *bytes, size_t len, char *out, bool prefix);

#ifdef __cplusplus
}
#endif

#endif /* HEX_H */
