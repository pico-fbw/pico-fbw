#pragma once

#include <stdbool.h>
#include "platform/int.h"

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
void display_power_save();

/**
 * Queues the display "bootup complete" animation to play.
 */
void display_anim();
