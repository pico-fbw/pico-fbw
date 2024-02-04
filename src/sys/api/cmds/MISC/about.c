/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <stdio.h>
#include "pico/platform.h"

#include "io/platform.h"

#include "sys/info.h"

#include "sys/api/cmds/MISC/about.h"

void api_about(const char *cmd, const char *args) {
    switch (platform_type()) {
        case PLATFORM_FBW:
            printf("Genuine pico-fbw v%s, API v%s, Wi-Fly v%s, RP2040-B%d\n\n",
                    PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version());
            break;
        case PLATFORM_PICO_W:
            printf("pico(w)-fbw v%s, API v%s, Wi-Fly v%s, RP2040-B%d\n\n",
                    PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version());
            break;
        default:
            // Generic message for unknown platforms and regular Pico
            printf("pico-fbw v%s, API v%s, Wi-Fly Unsupported, RP2040-B%d\n\n",
                    PICO_FBW_VERSION, PICO_FBW_API_VERSION, rp2040_chip_version());
            break;
    }
    printf("Copyright (C) 2023-2024 pico-fbw\n\n"
           "This program is free software: you can redistribute it and/or modify "
           "it under the terms of the GNU General Public License as published by "
           "the Free Software Foundation, either version 3 of the License, or "
           "(at your option) any later version.\n\n"

           "This program is distributed in the hope that it will be useful, "
           "but WITHOUT ANY WARRANTY; without even the implied warranty of "
           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
           "GNU General Public License for more details.\n\n"

           "You should have received a copy of the GNU General Public License "
           "along with this program. If not, see https://www.gnu.org/licenses/.\n");
}
