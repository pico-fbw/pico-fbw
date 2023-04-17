#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "io/flash.h"

#if !PICO_NO_FLASH
    #warning "It is advised to build the reset program into SRAM and not flash."
#endif
#ifndef PICO_DEFAULT_LED_PIN
    #warning "No default LED pin found. Status LED functionality may be impacted."
#endif

int main() {
    // Erase only the flash sectors containing pwm and pid calibration values
    flash_erase(0);
    flash_erase(1);
    // Flash LED a few times to indicate success
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    for (int i = 0; i < 10; ++i) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        sleep_ms(200);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        sleep_ms(200);
    }
    // Automatically reboot in BOOTSEL mode
    reset_usb_boot(0, 0);

    // How did we get here? (pt. 2)
    return 0;
}
