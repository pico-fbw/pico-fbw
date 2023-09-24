#ifndef __PLATFORM_H
#define __PLATFORM_H

#define WATCHDOG_TIMEOUT_MAGIC 0xAC0B3DED
#define WATCHDOG_FORCE_MAGIC 0xC0DE3298

#define MARBE_I i2c1
#define MARBE_D 18
#define MARBE_C 19
#define MARBE_FKZ 400
#define MARBE_R 0x74
#define MARBE_TUS 10000

void marbe_s(uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t l5, uint8_t l6);

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
 * Signal to the platform that the boot sequence is complete.
*/
void platform_boot_complete();

/**
 * Signal to the platform that a reboot is required.
 * @param type the type of reboot to perform.
*/
void platform_reboot(RebootType type);

/**
 * Signal to the platform that a shutdown is required.
 * @note This simply reboots the system in bootloader but quietly;
 * no mass storage is mounted, so it will appear as shut down.
*/
void platform_shutdown();

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
