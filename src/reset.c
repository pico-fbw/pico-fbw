/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "pico/bootrom.h"

#include "io/flash.h"

#if !PICO_NO_FLASH
    #warning It is advised to build the reset program into SRAM and not flash.
#endif

int main() {
    flash_format();
    reset_usb_boot(0, 0); // Reboot into BOOTSEL mode

    return 0; // How did we get here? (pt. 2)
}
