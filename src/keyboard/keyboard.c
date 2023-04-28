#include "../../include/keyboard.h"
#include "../../lib/lib-header/portio.h"
#include "../../include/framebuffer.h"
#include "../../lib/lib-header/stdmem.h"

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

static struct KeyboardDriverState keyboard_state = {
    .read_extended_mode = 0,
    .keyboard_input_on = 0,
    .buffer_index = 0,
    .keyboard_buffer = {[0 ... KEYBOARD_BUFFER_SIZE -1] = 0 }
};

uint32_t BUFFER_COUNT = 0;

void keyboard_state_activate(void) {
    keyboard_state.keyboard_input_on = 1;
}

void keyboard_state_deactivate(void) {
    keyboard_state.keyboard_input_on = 0;
}

void get_keyboard_buffer(char *buf) {
    int i;
    for (i = 0; i < KEYBOARD_BUFFER_SIZE; i++) {
        buf[i] = keyboard_state.keyboard_buffer[i];
        keyboard_state.keyboard_buffer[i] = '\0';
    }
    keyboard_state.buffer_index = 0;
}

bool is_keyboard_blocking(void) {
    return keyboard_state.keyboard_input_on;
}

void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on) {
        keyboard_state.buffer_index = 0;
    }
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        uint8_t row;
        uint8_t col;
        // Handle enter character
        if (mapped_char == '\n') {
            // Set cursor
            uint32_t temp = BUFFER_COUNT + (80 - (BUFFER_COUNT % 80));
            row = temp / 80;
            col = temp % 80;
            if (row <= 24) {
                BUFFER_COUNT = temp;
                framebuffer_set_cursor(row, col);
            }
            // Stop processing scancodes when enter key is pressed
            keyboard_state.keyboard_input_on = 0;
            // keyboard_state.buffer_index = 0;
        // Handle backspace character
        } else if (mapped_char == '\b') {
            if (BUFFER_COUNT > 0) {    
                    // Remove character from buffer
                    keyboard_state.buffer_index--;
                    keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
                    BUFFER_COUNT--;
                    row = BUFFER_COUNT / 80;
                    col = BUFFER_COUNT % 80;
                    // Skip over empty character
                    while (*(MEMORY_FRAMEBUFFER + (row * 80 + col) * 2) == 0x00) {
                        if (col > 0) {
                            BUFFER_COUNT--;
                            col--;
                        }
                        else {
                            row--;
                            col = 79;
                            BUFFER_COUNT--;
                        }
                    }
                    framebuffer_clear_char(row, col);
                    framebuffer_set_cursor(row, col);
            } else {
                framebuffer_set_cursor(0, 0);
            }
        // Handle printable character
        } else if (mapped_char != 0) {
            // Add mapped character to buffer
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
            keyboard_state.buffer_index++;
            row = BUFFER_COUNT / 80;
            col = BUFFER_COUNT % 80;
            // Boundary for keyboard write
            if (!(row == 24 && col == 79)) {
                framebuffer_write(row, col, mapped_char, 0xF, 0);
                BUFFER_COUNT++;
                row = BUFFER_COUNT / 80;
                col = BUFFER_COUNT % 80;
            }
            framebuffer_set_cursor(row, col);
        }
    }
    pic_ack(IRQ_KEYBOARD);
}