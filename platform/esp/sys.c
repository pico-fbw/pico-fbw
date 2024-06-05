/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <assert.h>
#include "esp_sleep.h"    // https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/sleep_modes.html
#include "esp_system.h"   // https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/misc_system_api.html
#include "esp_task_wdt.h" // https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/wdts.html
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "platform/sys.h"

#define THREAD_DELAY_MS 10 // The amount of time to allow for other RTOS tasks to run

void sys_boot_begin() {
    // Initialize NVS, used to store wifi data
    esp_err_t init = nvs_flash_init();
    if (init == ESP_ERR_NVS_NO_FREE_PAGES || init == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        assert(nvs_flash_erase() == ESP_OK);
        init = nvs_flash_init();
    }
    assert(init == ESP_OK);
}

void sys_boot_end() {
    // Watchdog is automatically initialized by bootloader, so we just need to subscribe to it
    esp_task_wdt_add(NULL);
}

void sys_periodic() {
    if (esp_task_wdt_status(NULL) == ESP_OK)
        esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(THREAD_DELAY_MS)); // Allow other RTOS tasks to run
    static_assert(pdMS_TO_TICKS(THREAD_DELAY_MS) > 0, "THREAD_DELAY_MS must be at least one tick");
}

void __attribute__((noreturn)) sys_shutdown() {
    esp_task_wdt_deinit();
    esp_deep_sleep(UINT32_MAX); // UINT64_MAX seems to be out of range, and UINT32_MAX is pretty long either way
}

void __attribute__((noreturn)) sys_reboot(bool bootloader) {
    esp_restart();
    while (true)
        ;
    (void)bootloader; // ESP does not support rebooting into bootloader
}

BootType sys_boot_type() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:
            return BOOT_COLD;
        case ESP_RST_SW:
            return BOOT_REBOOT;
        case ESP_RST_PANIC:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
            return BOOT_WATCHDOG;
        case ESP_RST_EXT:
        default:
            return BOOT_RESET;
    }
}
