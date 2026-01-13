#include "hex.h"
#include <string.h>
#include <ctype.h>

bool is_hex_char(char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

bool is_hex_string(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (!is_hex_char(str[i])) {
            return false;
        }
    }
    return true;
}

bool has_hex_prefix(const char *str) {
    return str && strlen(str) >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X');
}

uint8_t hex_char_to_nibble(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    return 0;
}

int hex_to_bytes(const char *hex, uint8_t *out, size_t hex_len) {
    const char *start = hex;
    size_t len = hex_len;
    
    /* Skip 0x prefix */
    if (has_hex_prefix(hex)) {
        start += 2;
        len -= 2;
    }
    
    /* Must be even length */
    if (len % 2 != 0) {
        return -1;
    }
    
    /* Check all chars are hex */
    if (!is_hex_string(start, len)) {
        return -1;
    }
    
    size_t out_len = len / 2;
    for (size_t i = 0; i < out_len; i++) {
        uint8_t high = hex_char_to_nibble(start[i * 2]);
        uint8_t low = hex_char_to_nibble(start[i * 2 + 1]);
        out[i] = (uint8_t)((high << 4) | low);
    }
    
    return (int)out_len;
}

void bytes_to_hex(const uint8_t *bytes, size_t len, char *out, bool prefix) {
    static const char hex_chars[] = "0123456789abcdef";
    size_t offset = 0;
    
    if (prefix) {
        out[0] = '0';
        out[1] = 'x';
        offset = 2;
    }
    
    for (size_t i = 0; i < len; i++) {
        out[offset + i * 2] = hex_chars[(bytes[i] >> 4) & 0x0F];
        out[offset + i * 2 + 1] = hex_chars[bytes[i] & 0x0F];
    }
    
    out[offset + len * 2] = '\0';
}
