#include "pico/bootrom.h"

#include "io/flash.h"

#if !PICO_NO_FLASH
    #warning It is advised to build the reset program into SRAM and not flash.
#endif

int main() {
    // Erase only the flash sectors containing pwm and pid calibration values
    for (uint8_t i = 0; i < 3; i++) {
        flash_erase(i);
    }
    // Automatically reboot in BOOTSEL mode
    reset_usb_boot(0, 0);

    // How did we get here? (pt. 2)
    return 0;
}
