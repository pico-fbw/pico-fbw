// Huge thank-you to Raspberry Pi (as a part of the Pico examples library) for giving me a head start on the display code!

/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 * 
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/types.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "../sys/config.h"

#include "display.h"

static uint8_t buf[DISPLAY_BUF_LEN];

static RenderArea frame_area = {
    col_start: 0,
    col_end : DISPLAY_WIDTH - 1,
    page_start : 0,
    page_end : DISPLAY_NUM_PAGES - 1
};

/**
 * Sends a command to the display.
 * @param cmd The command to send
 * @return The return code of the i2c_write_timeout_us call.
*/
static inline int sendCmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    return i2c_write_timeout_us(DISPLAY_I2C, (DISPLAY_ADDR & DISPLAY_WRITE_MODE), buf, 2, false, DISPLAY_TIMEOUT_US);
}

/**
 * Sends a list of commands to the display.
 * @param buf The buffer containing the commands
 * @param num The number of commands to send
 * @return true if the commands were sent successfully, false otherwise.
*/
static bool sendCmdList(uint8_t *buf, int num) {
    for (uint i = 0; i < num; i++) {
        if (sendCmd(buf[i]) != 2) return false;
    }
}

/**
 * Sends a buffer to the display.
 * @param buf The buffer to send
 * @param buf_len The length of the buffer
*/
static void sendBuf(uint8_t buf[], int buf_len) {
    // Copy into a new buffer to add the control byte to the beginning
    uint8_t *temp_buf = malloc(buf_len + 1);
    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buf_len);

    i2c_write_timeout_us(DISPLAY_I2C, (DISPLAY_ADDR & DISPLAY_WRITE_MODE), temp_buf, buf_len + 1, false, DISPLAY_TIMEOUT_US);
    free(temp_buf);
}

/**
 * Updates a portion of the display with a render area.
 * @param buf The buffer to update with
 * @param area The render area to update
*/
static void render(uint8_t *buf, RenderArea *area) {
    uint8_t cmds[] = {
        DISPLAY_SET_COL_ADDR,
        area->col_start,
        area->col_end,
        DISPLAY_SET_PAGE_ADDR,
        area->page_start,
        area->page_end
    };
    sendCmdList(cmds, count_of(cmds));
    sendBuf(buf, area->buf_len);
}

/**
 * Calculates the length of the flattened buffer for a render area.
 * @param area The render area to calculate the buffer length for
*/
static void render_calcBuf(RenderArea *area) {
    area->buf_len = (area->col_end - area->col_start + 1) * (area->page_end - area->page_start + 1);
}

/**
 * @param ch The character to get the index for
 * @return the index of a character in the display font.
*/
static inline int GetFontIndex(uint8_t ch) {
    if (ch >= 'A' && ch <='Z') {
        return ch - 'A' + 1;
    } else if (ch >= '0' && ch <='9') {
        return ch - '0' + 27;
    } else if (ch == '-') {
        return 37;
    } else if (ch == '_') {
        return 38;
    } else if (ch == 254) {
        return 39;
    } else {
        return 0; // Not in the font, return blank
    }
}

/**
 * Sets a pixel on the display.
 * @param buf The buffer to update
 * @param x The x coordinate of the pixel
 * @param y The y coordinate of the pixel
 * @param on Whether the pixel is on or off
 * @note Ensure to render() the display once all pixels have been updated.
*/
static void buf_setPixel(uint8_t *buf, int x, int y, bool on) {
    assert(x >= 0 && x < DISPLAY_WIDTH && y >=0 && y < DISPLAY_HEIGHT);

    // The VRAM on the SSD1306 is split up in to 8 rows, one bit per pixel
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x = 0, y = 0->7, byte 1 is x = 1, y = 0->7 etc

    const int BytesPerRow = DISPLAY_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on) {
        byte |=  1 << (y % 8);
    } else {
        byte &= ~(1 << (y % 8));
    }
    buf[byte_idx] = byte;
}

/**
 * Draws a basic Bresnham line on the display.
 * @param buf The buffer to update
 * @param x0 The x coordinate of the start point
 * @param y0 The y coordinate of the start point
 * @param x1 The x coordinate of the end point
 * @param y1 The y coordinate of the end point
 * @param on Whether the line is on or off
 * @note Ensure to render() the display once all lines have been drawn.
*/
static void buf_drawLine(uint8_t *buf, int x0, int y0, int x1, int y1, bool on) {

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;

    while (true) {
        buf_setPixel(buf, x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * Writes a character to the display.
 * @param buf The buffer to update
 * @param x The x coordinate of the character
 * @param y The y coordinate of the character
 * @param ch The character to write
 * @note Ensure to render() the display once all characters have been written.
*/
static void buf_writeChar(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x > DISPLAY_WIDTH - 8 || y > DISPLAY_HEIGHT - 8) return;
    y = y / 8; // Only write on Y row boundaries (every 8 vertical pixels)

    ch = toupper(ch);
    int idx = GetFontIndex(ch);
    int fb_idx = y * 128 + x;

    for (uint i = 0; i < 8; i++) {
        buf[fb_idx++] = font[idx * 8 + i];
    }
}

/**
 * Writes a string to the display.
 * @param buf The buffer to update
 * @param x The x coordinate of the string
 * @param y The y coordinate of the string
 * @param str The string to write
 * @note Ensure to render() the display once all strings have been written.
*/
static void buf_writeString(uint8_t *buf, int16_t x, int16_t y, char *str) {
    // Cull out any string off the screen
    if (x > DISPLAY_WIDTH - 8 || y > DISPLAY_HEIGHT - 8) return;

    while (*str) {
        buf_writeChar(buf, x, y, *str++);
        x += 8;
    }
}

/**
 * Centers a string of text.
 * @param line The string to center
 * @param len_max The maximum length of the string
 * @return The centered string.
 * @note This function allocates memory for the centered string, ensure to free() it after use.
*/
static char *centerString(char line[], uint len_max) {
    uint len = strlen(line);
    if (len < len_max) {
        // Create line buffer to be centered and set to spaces (padding)
        char *centered = calloc(len_max, sizeof(char));
        if (centered == NULL) return NULL;
        memset(centered, ' ', len_max);
        // Calculate padding needed
        uint padding = (len_max - len) / 2;
        // Add text back between padding
        for (uint i = padding; i < len_max - padding; i++) {
            centered[i] = line[i - padding];
        }
        return centered;
    }
    return strdup(line); // No centering needed
}


bool display_init() {
    if (config.debug.debug_fbw) printf("[display] initializing ");
    if (DISPLAY_I2C == i2c0) {
        if (config.debug.debug_fbw) printf("i2c0\n");
    } else if (DISPLAY_I2C == i2c1) {
        if (config.debug.debug_fbw) printf("i2c1\n");
    } else {
        if (config.debug.debug_fbw) printf("\n");
    }
    i2c_init(DISPLAY_I2C, DISPLAY_FREQ_KHZ * 1000);
    gpio_set_function(DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA);
    gpio_pull_up(DISPLAY_SCL);

    uint8_t cmds[] = {
        DISPLAY_SET_DISP,               // Display off
        /* Memory mapping */
        DISPLAY_SET_MEM_MODE,           // Set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                           // Horizontal addressing mode
        /* Resolution and layout */
        DISPLAY_SET_DISP_START_LINE,    // Set display start line to 0
        DISPLAY_SET_SEG_REMAP | 0x01,   // Set segment re-map, column address 127 is mapped to SEG0
        DISPLAY_SET_MUX_RATIO,          // Set multiplex ratio
        DISPLAY_HEIGHT - 1,             // Display height - 1
        DISPLAY_SET_COM_OUT_DIR | 0x08, // Set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
        DISPLAY_SET_DISP_OFFSET,        // Set display offset (none)
        0x00,
        DISPLAY_SET_COM_PIN_CFG,        // Set COM (common) pins hardware configuration. Board specific magic number. 
                                        // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
        #if ((DISPLAY_WIDTH == 128) && (DISPLAY_HEIGHT == 32))
        0x02,                           
        #elif ((DISPLAY_WIDTH == 128) && (DISPLAY_HEIGHT == 64))
        0x12,
        #else
        0x02,
        #endif
        /* Timing and driving scheme */
        DISPLAY_SET_DISP_CLK_DIV,       // Set display clock divide ratio
        0x80,                           // Div ratio of 1, standard freq
        DISPLAY_SET_PRECHARGE,          // Set pre-charge period
        0xF1,                           // VCC internally generated
        DISPLAY_SET_VCOM_DESEL,         // Set VCOMH deselect level
        0x30,                           // 0.83 x VCC
        /* display */
        DISPLAY_SET_CONTRAST,           // Set contrast control
        0xFF,
        DISPLAY_SET_ENTIRE_ON,          // Set entire display on to follow RAM content
        DISPLAY_SET_NORM_DISP,          // Set normal (not inverted) display
        DISPLAY_SET_CHARGE_PUMP,        // Set charge pump
        0x14,                           // VCC internally generated
        DISPLAY_SET_SCROLL | 0x00,      // Deactivate horizontal scrolling if set, memory writes will corrupt otherwise
        DISPLAY_SET_DISP | 0x01,        // Turn display on
    };
    if (!sendCmdList(cmds, count_of(cmds))) return false;

    // Zero the display
    memset(buf, 0, DISPLAY_BUF_LEN);
    // Calculate render area for logo and display
    frame_area.col_start = 0;
    frame_area.col_end = LOGO_WIDTH - 1;
    render_calcBuf(&frame_area);
    render(logo, &frame_area);
    // Reset render area back for text (but do not render yet)
    frame_area.col_end = DISPLAY_WIDTH - 1;
    render_calcBuf(&frame_area);
    return true;
}

void display_text(char l1[], char l2[], char l3[], char l4[], bool center) {
    memset(buf, 0, DISPLAY_BUF_LEN); // Clear anything that may be in VRAM buffer
    char *text[4];
    text[0] = center ? centerString(l1, DISPLAY_MAX_LINE_LEN) : l1;
    text[1] = center ? centerString(l2, DISPLAY_MAX_LINE_LEN) : l2;
    text[2] = center ? centerString(l3, DISPLAY_MAX_LINE_LEN) : l3;
    text[3] = center ? centerString(l4, DISPLAY_MAX_LINE_LEN) : l4;

    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        if (text[i] == NULL) continue;
        buf_writeString(buf, 5, y, text[i]);
        free(text[i]);
        y += 8;
    }
    render(buf, &frame_area);
}
