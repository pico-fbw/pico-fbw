/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "esp_sleep.h" // https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/sleep_modes.html
#include "esp_timer.h" // https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/esp_timer.html
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "platform/time.h"

// Wrapper function to convert the callback signature from `i32 (*)()` to `esp_timer_cb_t` and handle reschedule logic
static void callback_to_esp_timer_cb_t(void *arg) {
    // Derive the original callback function and its id from the data (passed in when creating the timer)
    CallbackData *data = (CallbackData *)arg;
    if (!data)
        return;
    // Run the specified callback function which should return either zero or a number of milliseconds to reschedule the
    // callback
    i32 reschedule = data->callback();
    if (reschedule > 0) {
        esp_timer_start_once(data->id, reschedule * 1000);
    } else {
        esp_timer_delete(data->id);
        free(data);
    }
}

u64 time_us() {
    return esp_timer_get_time();
}

CallbackData *callback_in_ms(u32 ms, Callback callback) {
    // Create a new CallbackData to store the callback with its corresponding id (so it can be used later to reschedule/cancel
    // the callback)
    CallbackData *data = malloc(sizeof(CallbackData));
    if (!data)
        return NULL;
    data->callback = callback;
    // Use esp_timer to schedule the callback and return its id
    const esp_timer_create_args_t timer = {
        .callback = callback_to_esp_timer_cb_t,
        .arg = (void *)data, // Our data will be stored in the esp timer and later passed to the wrapper function
        .dispatch_method = ESP_TIMER_TASK,
        .skip_unhandled_events = false,
    };
    esp_timer_handle_t handle;
    if (esp_timer_create(&timer, &handle) != ESP_OK) {
        free(data);
        return NULL;
    }
    if (esp_timer_start_once(handle, ms * 1000) != ESP_OK) {
        esp_timer_delete(handle);
        free(data);
        return NULL;
    }
    data->id = handle;
    return data;
}

void cancel_callback(CallbackData *data) {
    // Check if the data is valid (it is possible that the callback has been automatically deleted); this prevents a double free
    if (!data)
        return;
    esp_timer_stop(data->id);
    esp_timer_delete(data->id);
    free(data);
}

void sleep_us_blocking(u64 us) {
    const TickType_t delay = pdMS_TO_TICKS(us / 1000);
    if (delay > 0 && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        // Delay the current task by allowing other tasks to run
        vTaskDelay(delay);
    else {
        // Too short to use FreeRTOS, use hardware sleep instead
        esp_sleep_enable_timer_wakeup(us);
        esp_light_sleep_start();
    }
}
