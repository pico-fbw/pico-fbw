#pragma once

#include <stdbool.h>
#include "platform/int.h"

// Display dimensions
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32
#define DISPLAY_MAX_LINE_LEN 15 // 15 for margins; 16 can physically be fit but it looks bad

// Display i2c information
#define DISPLAY_FREQ_KHZ 400
#define DISPLAY_ADDR 0x3C
#define DISPLAY_SDA 18
#define DISPLAY_SCL 19
// TODO: move these to platform/defs.h

// Datasheet: https://cdn-shop.adafruit.com/datasheets/DISPLAY.pdf
// Commands:
#define DISPLAY_SET_MEM_MODE 0x20
#define DISPLAY_SET_COL_ADDR 0x21
#define DISPLAY_SET_PAGE_ADDR 0x22
#define DISPLAY_SET_HORIZ_SCROLL 0x26
#define DISPLAY_SET_SCROLL 0x2E

#define DISPLAY_SET_DISP_START_LINE 0x40

#define DISPLAY_SET_CONTRAST 0x81
#define DISPLAY_SET_CHARGE_PUMP 0x8D

#define DISPLAY_SET_SEG_REMAP 0xA0
#define DISPLAY_SET_ENTIRE_ON 0xA4
#define DISPLAY_SET_ALL_ON 0xA5
#define DISPLAY_SET_NORM_DISP 0xA6
#define DISPLAY_SET_INV_DISP 0xA7
#define DISPLAY_SET_MUX_RATIO 0xA8
#define DISPLAY_SET_DISP 0xAE
#define DISPLAY_SET_COM_OUT_DIR 0xC0
#define DISPLAY_SET_COM_OUT_DIR_FLIP 0xC0

#define DISPLAY_SET_DISP_OFFSET 0xD3
#define DISPLAY_SET_DISP_CLK_DIV 0xD5
#define DISPLAY_SET_PRECHARGE 0xD9
#define DISPLAY_SET_COM_PIN_CFG 0xDA
#define DISPLAY_SET_VCOM_DESEL 0xDB

#define DISPLAY_PAGE_HEIGHT 8
#define DISPLAY_NUM_PAGES (DISPLAY_HEIGHT / DISPLAY_PAGE_HEIGHT)
#define DISPLAY_BUF_LEN (DISPLAY_NUM_PAGES * DISPLAY_WIDTH)

#define DISPLAY_WRITE_MODE 0xFE
#define DISPLAY_READ_MODE 0xFF

/**
 * Initializes the display.
 * @return true if the display was initialized successfully, false otherwise.
 */
bool display_init();

/**
 * Renders lines of text on the display.
 * @param l1 The first line of text
 * @param l2 The second line of text
 * @param l3 The third line of text
 * @param l4 The fourth line of text
 * @param center Whether to center the text (false leaves it as-is)
 * @note The display supports 4 lines of text, with a maximum of ~15 characters per line.
 * Any more characters will be truncated.
 */
void display_lines(const char l1[], const char l2[], const char l3[], const char l4[], bool center);

/**
 * Renders a string on the display.
 * @param str The string to display
 * @param progress The progress of am optional progress bar (0-100), or -1 if not needed
 * @note This function will try its best to wrap words between lines, but it's not perfect, so use display_lines() if you can.
 * The maximum string length is ~60 characters (or 45 is a progress bar is used), any more will be truncated.
 */
void display_string(const char *str, i32 progress);

/**
 * Puts the display into "power save" mode, where a small note is shortly displayed and then the screen is shut off.
 */
void display_powerSave();

/**
 * Queues the display "bootup complete" animation to play.
 * @note The animation will only succeed if there are no errors at present.
 */
void display_anim();
