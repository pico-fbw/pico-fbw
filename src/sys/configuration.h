#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H

#include <stdbool.h>

/* The amount of time (in ms) to wait for any possible serial connections to be established before booting.
This option is compiled in, as configuration is not yet loaded when this value is needed. */
#define BOOT_WAIT_MS 1000

typedef enum ConfigSectionType {
    SECTION_TYPE_NONE,
    SECTION_TYPE_FLOAT,
    SECTION_TYPE_STRING
} ConfigSectionType;

typedef enum ConfigSection {
    CONFIG_GENERAL,
    CONFIG_CONTROL,
    CONFIG_PINS,
    CONFIG_SENSORS,
    CONFIG_WIFLY,
    CONFIG_SYSTEM,
    CONFIG_PID,
} ConfigSection;

#define CONFIG_GENERAL_STR "General"
#define CONFIG_CONTROL_STR "Control"
#define CONFIG_PINS_STR "Pins"
#define CONFIG_SENSORS_STR "Sensors"
#define CONFIG_WIFLY_STR "WiFly"
#define CONFIG_SYSTEM_STR "System"
#define CONFIG_PID_STR "PID" // Comes out of order

#define NUM_FLOAT_CONFIG_SECTIONS 6
#define NUM_STRING_CONFIG_SECTIONS 1
#define NUM_CONFIG_SECTIONS (NUM_FLOAT_CONFIG_SECTIONS + NUM_STRING_CONFIG_SECTIONS)

/**
 * @return Whether the current config is valid.
*/
bool config_validate();

/**
 * Gets a value from the config based on its string representation.
 * @param section The name of the section to look in
 * @param key The name of the key to look up
 * @param value The pointer to the value to store the result in
 * The pointer will point to either a float or a char depending on the...
 * @return type of the value stored.
*/
ConfigSectionType config_get(const char *section, const char *key, void **value);

/**
 * Sets a value in the config based on its string representation.
 * @param section The name of the section to look in
 * @param key The name of the key to look up
 * @param value The pointer to the value to store
 * The pointer will point to either a float or a char depending on the section type/user input.
 * @return Whether the value was successfully set.
*/
bool config_set(const char *section, const char *key, const char *value);

/**
 * Gets a string representation of a config section based on its (enum) index, as well as its type.
 * @param section The (enum) index of the section
 * @param str The pointer to the string to store the result in
 * @return The type of the section
*/
ConfigSectionType config_sectionToString(ConfigSection section, const char **str);

#endif // __CONFIGURATION_H