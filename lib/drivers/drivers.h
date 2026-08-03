#pragma once

#include "platform/types.h"

#include "sys/configuration.h"

#define BUSTYPE_MIN BUS_I2C
typedef enum BusType {
    BUS_I2C,
    BUS_SPI,
    BUS_ALL,
} BusType;
#define BUSTYPE_MAX BUS_ALL

typedef struct BusConfig {
    BusType type;
    union {
        struct {
            byte addr;
        } i2c;
        struct {
            i16 cs;
        } spi;
    };
} BusConfig;

typedef struct FusionDriver FusionDriver; // Forward declaration
typedef struct FusionDriver {
    BusConfig bus; // Bus configuration on how to communicate with the device
    void *context; // Arbitrary context that a driver may need
    /**
     * @param self pointer to the driver
     * @return true if this device is present and ready to be initialized
     * @note This function also sets the `addr` field of the driver if it succeeds.
     */
    bool (*exists)(FusionDriver *self);
    /**
     * Initializes the driver, including all needed internal configuration.
     * @param self pointer to the driver
     * @return true if the initialization was successful
     */
    bool (*init)(FusionDriver *self);
    /**
     * Should read data from the device and store it in the data array.
     * @param self pointer to the driver
     * @param data array to store the read data, formatted as a 3-element vector
     * (see the FusionDevice struct for details on this vector's format)
     * @return true if the read was successful
     */
    bool (*read)(FusionDriver *self, f32 data[3]);
    /**
     * Deinitializes the driver and frees any resources it may have allocated.
     * @param self pointer to the driver
     * @note This function is optional and may be NULL if not needed.
     */
    void (*destroy)(FusionDriver *self);
} FusionDriver;

typedef struct FusionDevice {
    // One or more drivers may be NULL
    FusionDriver *acc, *gyro, *mag, *baro;
    f32 accData[3];  // x, y, z (m/s^2)
    f32 gyroData[3]; // x, y, z (rad/s)
    // Unioned to save space since mag and baro will never be in the same sensor
    union {
        f32 magData[3];  // x, y, z (uT)
        f32 baroData[3]; // pressure (Pa), temperature (°C), %RH (% 0-1)
    };
    const char *name; // Human-readable identifier for the device
} FusionDevice;

extern FusionDevice bmi323;

extern FusionDevice *fusionDevices[];
extern const u32 numFusionDevices;

/**
 * A function provided to `check_devid()` to generalize device verification.
 * @param bus device's bus configuration
 * @param reg the device ID register to read from
 * @return the byte that was read, or `0x00` if reading failed
 */
typedef byte (*check_devid_fn)(BusConfig *bus, byte reg);

/**
 * Checks if a driver exists and attempts to initialize it if it does.
 * @param device pointer to the device
 * @param driver pointer to the driver to initialize
 * @param name human-readable identifier for the driver name
 * @return false if the driver does not exist or failed to initialize
 */
bool init_driver(const FusionDevice *device, FusionDriver *driver, const char *name);

/**
 * Deinitializes a driver, if applicable.
 * @param device pointer to the device
 * @param driver pointer to the driver to deinitialize
 * @param name human-readable identifier for the driver name
 */
void deinit_driver(const FusionDevice *device, FusionDriver *driver, const char *name);

/**
 * Check that a device's ID matches what is expected, to confirm its existence and identity.
 * @param bus device's bus configuration (will be mutated)
 * @param addr primary I2C address
 * @param alt_addr alternate I2C address
 * @param reg device ID register
 * @param expected expected value of the device ID register
 * @param check_fn function to read the device ID
 * @note `check_fn()` can be a custom function if dummy bytes or other forms of communication are involved,
 * but in many cases, the generic `driver_read_byte()` can be used.
 * @note If the function returns `true`, the configuration of `bus` may be re-used to continue communication with the
 * device.
 */
bool check_devid(BusConfig *bus, byte addr, byte alt_addr, byte reg, byte expected, check_devid_fn check_fn);

/**
 * Reads `len` bytes from `reg` and stores them in `dest[]`.
 * @param bus device's bus configuration
 * @param reg register to read from
 * @param dest buffer to read data into
 * @param len number of bytes to read
 * @return true if the read was successful
 * @note `dest[]` must be large enough to hold `len` bytes of data.
 */
bool driver_read(BusConfig *bus, byte reg, byte dest[], size_t len);

/**
 * Writes `len` bytes from `src[]` at `reg`.
 * @param reg register to write to
 * @param src buffer of data to write
 * @param len number of bytes to write
 * @return true if the write was successful
 * @note `src[]` must contain at least `len` bytes of data.
 */
bool driver_write(BusConfig *bus, byte reg, const byte src[], size_t len);

static inline byte driver_read_byte(BusConfig *bus, byte reg) {
    byte data = 0x00;
    driver_read(bus, reg, &data, 1);
    return data;
}

static inline bool driver_write_byte(BusConfig *bus, byte reg, byte data) {
    return driver_write(bus, reg, (byte[]){data}, 1);
}

static inline word driver_read_word(BusConfig *bus, byte reg) {
    byte raw[2] = {};
    driver_read(bus, reg, raw, sizeof(raw));
    return (word)(raw[1] << 8 | raw[0]);
}

static inline bool driver_write_word(BusConfig *bus, byte reg, word data) {
    return driver_write(bus, reg, (byte[]){data & 0xFF, (data >> 8) & 0xFF}, 2);
}

static inline byte driver_read_bits(BusConfig *bus, byte reg, byte mask) {
    byte value = driver_read_byte(bus, reg);
    return value & mask;
}

static inline bool driver_write_bits(BusConfig *bus, byte reg, byte mask, byte data) {
    byte value = driver_read_byte(bus, reg);
    value &= ~mask;
    value |= data & mask;
    return driver_write_byte(bus, reg, value);
}

static inline word driver_read_bits_word(BusConfig *bus, byte reg, word mask) {
    word value = driver_read_word(bus, reg);
    return value & mask;
}

static inline bool driver_write_bits_word(BusConfig *bus, byte reg, word mask, word data) {
    word value = driver_read_word(bus, reg);
    value &= ~mask;
    value |= data & mask;
    return driver_write_word(bus, reg, value);
}
