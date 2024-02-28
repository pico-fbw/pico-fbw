#pragma once

#include <stdbool.h>
#include "platform/sys.h"

/**
 * Runs beginning of boot tasks, should be called at the start of `main()`.
 */
void boot_begin();

/**
 * Sets the boot progress and message, and updates the display if available.
 *
 * @param progress the progress percentage (0-100)
 * @param message the message to display
 */
void boot_set_progress(float progress, const char *message);

/**
 * Runs end of boot tasks, should be called at the end of `main()`.
 */
void boot_complete();

/**
 * @return true if the boot process is completed
 */
bool boot_is_booted();

/**
 * @return the type of boot that just occurred
 * @note Must be called between `boot_begin()` and `boot_complete()`.
 */
inline BootType boot_type() { return sys_boot_type(); }
