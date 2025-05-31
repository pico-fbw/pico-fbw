/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "drivers.h"

// Here would be a good place for any #defines or #includes needed for the driver,
// any helper functions, and/or a link to its datasheet.

bool example_exists(FusionDriver *self) {
    // This function should only check to see that the device this driver is intended for is connected and responding;
    // it must not take any action that would change the state of the device.
    // If this function succeeds, it must set the `addr` field of the driver to the address of the detected device,
    // and return true. If the device is not detected, it should return false.
    // drivers.h has a good helper function `check_devid()` that may prove useful here.
}

bool example_init(FusionDriver *self) {
    // This function should initialize the device and prepare it for reading.
    // It should set up any necessary configuration, such as setting the device's mode, range, etc.
    // Additionally, it should allocate any resources that the driver will need to read data from the device.
    // If the initialization is successful, it should return true. If it fails, it should return false.
}

bool example_read(FusionDriver *self, f32 data[]) {
    // This function should read data from the device and store it in the `data` array.
    // The data should be stored in the order specified in the `FusionDevice` struct in drivers.h.
    // If the read is successful, it should return true. If it fails, it should return false.
}

void example_destroy(FusionDriver *self) {
    // This function should free any resources that were allocated in the `init` function.
    // If the driver does not allocate any resources, this function may be deleted from the driver.
}

// This is the `FusionDevice` struct that the driver will use to register itself with the fusion system.
// The `name` field should be a unique name for the device, and the `acc`, `gyro`, `mag`, and `baro` fields
// should point to appropriate `FusionDriver` structs.
// If the device does not have a particular sensor, the corresponding field should be set to NULL or not initialized.
const FusionDevice example = {
    .acc = &((FusionDriver){
        .exists = example_exists,
        .init = example_init,
        .read = example_read,
        .destroy = example_destroy,
    }),
    // This same setup can be repeated for .gyro, .mag, and .baro if the device has those sensors.
    .name = "Example",
};

// After configuring the FusionDevice struct, the driver should be registered with the fusion system.
// This can be done by adding the line `extern const FusionDevice example;` to drivers.h, changing the name accordingly.
// Then, the driver should be added to the `fusionDevices` array in drivers.c.
// Finally, the driver source file should be added to the CMakelists.txt file (located in lib/CMakeLists.txt).
// The driver will then be automatically detected and utilized by the fusion system.
