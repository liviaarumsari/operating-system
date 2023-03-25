#include "../../lib/lib-header/stdtype.h"
#include "../../lib/lib-header/stdmem.h"
#include "../../lib/lib-header/portio.h"
#include "../../include/framebuffer.h"

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // Determine buffer index
    uint16_t* index = (uint16_t*)(MEMORY_FRAMEBUFFER + (row * 80 + col) * 2);
    
    // Setting the value on index as 16-bit integer
    *index = ((bg << 12) | (fg << 8) | c);
}

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // Determine buffer position
    uint16_t position = r * 80 + c;

    // Set the high byte
    out(CURSOR_PORT_CMD, 0x0E);
    out(CURSOR_PORT_DATA, (uint8_t)(position >> 8));

    // Set the low byte
    out(CURSOR_PORT_CMD, 0x0F);
    out(CURSOR_PORT_DATA, (uint8_t)(position & 0xFF));
}

void framebuffer_clear(void) {
    // Set all character cells in the framebuffer to the empty character (0x00)
    memset(MEMORY_FRAMEBUFFER, 0x00, 80 * 25 * 2);

    // Set character color to gray (0x07) and background color to black (0x00)
    for (uint16_t i = 1; i < 80 * 25 * 2; i += 2) {
        *((uint8_t*) MEMORY_FRAMEBUFFER + i) = 0x07;
    }
}
