#pragma once

#include <stdbool.h>
#include "platform/sys.h"
#include "platform/types.h"

/**
 * Runs beginning of boot tasks, should be called at the start of `main()`.
 */
void boot_begin();

/**
 * Mounts the root filesystem, used for configuration and other data.
 */
void boot_mount_fs();

/**
 * Checks for updates and applies them if necessary.
 */
void boot_do_updates();

/**
 * Initializes the receiver I/O.
 */
void boot_init_receiver();

/**
 * Initializes the servo I/O.
 */
void boot_init_servos();

/**
 * Initializes the ESC I/O.
 */
void boot_init_escs();

/**
 * Initializes the IMU sensor I/O.
 */
void boot_init_imu();

/**
 * Initializes the GPS I/O.
 */
void boot_init_gps();

/**
 * Initializes the Wi-Fi stack, if applicable on the current platform.
 */
void boot_init_wifi();

/**
 * Initializes the ADC I/O, if applicable on the current platform.
 */
void boot_init_adc();

/**
 * Runs end of boot tasks, should be called at the end of `main()`.
 */
void boot_complete();

/**
 * Sets the current boot progress and message.
 * @param progress the progress percentage (0-100)
 * @param message the message to display
 */
void boot_set_progress(f32 progress, const char *message);

/**
 * @return true if the boot process is completed
 */
bool boot_is_booted();

/**
 * @return the type of boot that just occurred
 * @note Must be called between `boot_begin()` and `boot_complete()`.
 */
static inline BootType boot_type() {
    return sys_boot_type();
}
