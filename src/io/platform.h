#ifndef __PLATFORM_H
#define __PLATFORM_H

#define MARBE_I i2c1
#define MARBE_D 18
#define MARBE_C 19
#define MARBE_FKZ 400
#define MARBE_R 0x74
#define MARBE_TUS 10000

void marbe_s(uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t l5, uint8_t l6);

void platform_boot_begin();

void platform_boot_complete();

/**
 * @note will return true if either a PICO or PICO_W is detected; this means with FBW and ATHENA as well
*/
bool platform_is_pico();

/**
 * @note will also return true if platform_is_athena()
*/
bool platform_is_fbw();

bool platform_is_athena();

/**
 * @return 0 PICO, 1 PICO_W, 2 FBW, 3 ATHENA, -1 ERROR
 * @note higher numbers will take precedence over lower numbers; for example, if PICO_W and FBW are both detected, FBW will be returned
*/
int platform_type();

#endif // __PLATFORM_H
