/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/defs.h"

#include "sys/print.h"
#include "sys/version.h"

#include "about.h"

i32 api_about(const char *args) {
    printraw("pico-fbw v%s, API v%s\n", PICO_FBW_VERSION, PICO_FBW_API_VERSION);
    printraw("Built on %s at %s (C%ld) for \"%s\", HAL v%s\n\n", __DATE__, __TIME__, __STDC_VERSION__, PLATFORM_NAME,
             PLATFORM_VERSION);
    printraw("Copyright (C) 2023-2024 pico-fbw\n\n"
             "This program is free software: you can redistribute it and/or modify "
             "it under the terms of the GNU Affero General Public License as published by "
             "the Free Software Foundation, either version 3 of the License, or "
             "(at your option) any later version.\n\n"

             "This program is distributed in the hope that it will be useful, "
             "but WITHOUT ANY WARRANTY; without even the implied warranty of "
             "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
             "GNU Affero General Public License for more details.\n\n"

             "You should have received a copy of the GNU Affero General Public License "
             "along with this program. If not, see https://www.gnu.org/licenses/.\n");
    return -1;
    (void)args;
}
