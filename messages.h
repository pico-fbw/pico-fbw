#ifndef __MESSAGES_H
#define __MESSAGES_H

#define APP_NAME "pico-fbw Configurator"

#define BTN_LOAD_PROJECT "Load Project"
#define FIELD_SELECT_MODEL "Select Model"
#define BTN_BUILD_UPLOAD "Build and Upload"

#define TOOLTIP_LOAD_PROJECT "Loads a project to edit"
#define TOOLTIP_SELECT_MODEL "Selects the Pico model to build for"
#define TOOLTIP_BUILD_UPLOAD "Builds and uploads the configured firmware to the Pico"

// Note: models must still be manually added to the menu bar, dropdown, and build function logic.
#define MODEL0_NAME "Pico"
#define MODEL0_ARGS "-DPICO_BOARD=pico"
#define MODEL1_NAME "Pico W"
#define MODEL1_ARGS "-DPICO_BOARD=pico_w"

enum Model {
    MODEL0,
    MODEL1
};

#endif // __MESSAGES_H
