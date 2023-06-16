/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "pico/bootrom.h"

#include "io/flash.h"

#if !PICO_NO_FLASH
    #warning It is advised to build the reset program into SRAM and not flash.
#endif

int main() {
    flash_reset();
    // Automatically reboot in BOOTSEL mode
    reset_usb_boot(0, 0);

    // How did we get here? (pt. 2)
    return 0;
}
