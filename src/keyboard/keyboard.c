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
    .keyboard_buffer = {0}
};

static uint32_t BUFFER_COUNT = 0;

void keyboard_state_activate(void) {
    keyboard_state.keyboard_input_on = 1;
}

void keyboard_state_deactivate(void) {
    keyboard_state.keyboard_input_on = 0;
}

void get_keyboard_buffer(char *buf) {
    int i;
    for (i = 0; i < KEYBOARD_BUFFER_SIZE && i < keyboard_state.buffer_index; i++) {
        buf[i] = keyboard_state.keyboard_buffer[i];
        keyboard_state.keyboard_buffer[i] = '\0';
    }
    keyboard_state.buffer_index = 0;
}

bool is_keyboard_blocking(void) {
    return keyboard_state.keyboard_input_on;
}

void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on)
        keyboard_state.buffer_index = 0;
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        if (mapped_char == '\n') {
            // Stop processing scancodes when enter key is pressed
            keyboard_state.keyboard_input_on = 0;
        } else {
            // Print character to screen and update cursor position
            if (mapped_char == '\b' && BUFFER_COUNT > 0) {
                // Handle backspace character
                BUFFER_COUNT--;
                uint8_t row = BUFFER_COUNT / 80;
                uint8_t col = BUFFER_COUNT % 80;
                framebuffer_set_cursor(row, col);
                framebuffer_clear_char(row, col);

            } else if (mapped_char != 0) {
                // Add mapped character to buffer
                keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
                keyboard_state.buffer_index++;
                // Handle printable character
                uint8_t row = BUFFER_COUNT / 80;
                uint8_t col = BUFFER_COUNT % 80;
                framebuffer_write(row, col, mapped_char, 0, 0xF);
                BUFFER_COUNT++;
                row = BUFFER_COUNT / 80;
                col = BUFFER_COUNT % 80;
                framebuffer_set_cursor(row, col);
            }
        }
    }
    pic_ack(IRQ_KEYBOARD);
}