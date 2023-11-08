#ifndef __PLATFORM_H
#define __PLATFORM_H

#define WATCHDOG_TIMEOUT_MAGIC 0xAC0B3DED
#define WATCHDOG_FORCE_MAGIC 0xC0DE3298

typedef enum Platform {
    PLATFORM_UNKNOWN,
    PLATFORM_PICO,
    PLATFORM_PICO_W,
    PLATFORM_FBW
} Platform;

typedef enum BootType {
    BOOT_NORMAL,
    BOOT_REBOOT,
    BOOT_WATCHDOG
} BootType;

typedef enum RebootType {
    REBOOT_FAST,
    REBOOT_BOOTLOADER
} RebootType;

/**
 * Signal to the platform that the boot sequence is starting.
*/
void platform_boot_begin();

/**
 * Signal to the platform at what progress% the boot sequence is at.
 * @param progress the progress of the boot sequence (from 0-100)
 * @param message the message to display
 * @note The message should be short (at most ~32 chars) to fit nicely.
*/
void platform_boot_setProgress(float progress, const char *message);

/**
 * @return the current status of the boot process--true if booted, false if not.
*/
bool platform_is_booted();

/**
 * Signal to the platform that the boot sequence is complete.
*/
void platform_boot_complete();

/**
 * Signal to the platform that a reboot is required.
 * @param type the type of reboot to perform
 * @note This function will not return, the function will continue to loop until the processor successfully reboots.
*/
void __attribute__((noreturn)) platform_reboot(RebootType type);

/**
 * Signal to the platform that a shutdown is required.
 * @note This simply reboots the system in bootloader but quietly;
 * no mass storage is mounted, so it will appear as shut down.
*/
void __attribute__((noreturn)) platform_shutdown();

/**
 * Gets the type of boot that occured.
*/
BootType platform_boot_type();

/**
 * @return true if a PICO or PICO_W is detected.
 * @note this means with FBW as well
*/
bool platform_is_pico();

/**
 * @return true if FBW is detected.
*/
bool platform_is_fbw();

/**
 * @return the detected platform type.
 * @note higher numbers will take precedence over lower numbers; for example, if PICO_W and FBW are both detected, FBW will be returned
*/
Platform platform_type();

#endif // __PLATFORM_H
