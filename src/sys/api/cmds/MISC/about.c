/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/defs.h"

#include "sys/print.h"
#include "sys/version.h"

#include "misc.h"

i32 api_about(const char *args) {
    printraw("pico-fbw [%s], API v%s\n", PICO_FBW_VERSION, API_VERSION);
    printraw("Built on %s at %s for %s [%s]\n\n", __DATE__, __TIME__, PLATFORM_NAME, PLATFORM_VERSION);
    printraw("Copyright (C) 2023-2026, pico-fbw\n\n"
             "This program is free software: you can redistribute it and/or modify\n"
             "it under the terms of the MIT License. See LICENSE for more details.\n\n"
             "This program comes with ABSOLUTELY NO WARRANTY; for details see the LICENSE file.\n\n"
             "For more information, visit https://pico-fbw.org\n\n");
    return -1;
    (void)args;
}
