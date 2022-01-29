#include "debug.h"

#include <stdint.h>


/**
 * @brief バイト列を16進数で出力する
 * @param data [IN]
 * @param len  [IN]
 */
void dump_bytes_hex(const uint8_t* data, uint8_t len) {
    if (len == 0) {
        DEBUG_PRINTF("(empty)");
        return;
    }
    const uint8_t* ptr = data;
    for (int i = 0; i < len; ++i) {
        if (i != 0) {
            DEBUG_PRINTF(", ");
        }
        // DEBUG_PRINTF("0x");
        // DEBUG_PRINTF(ptr[i], HEX);
        DEBUG_PRINTF("0x%02x", (uint8_t)ptr[i]);
    }
}

/**
 * @brief バイト列を10進数で出力する
 * @param data [IN]
 * @param len  [IN]
 */
void dump_bytes_dec(const uint8_t* data, uint8_t len) {
    if (len == 0) {
        DEBUG_PRINTF("(empty)");
        return;
    }
    const uint8_t* ptr = data;
    for (int i = 0; i < len; ++i) {
        if (i != 0) {
            DEBUG_PRINTF(", ");
        }
        // DEBUG_PRINTF(ptr[i], DEC);
        DEBUG_PRINTF("%d", (uint8_t)ptr[i]);
    }
}
