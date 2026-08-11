#pragma once

#include "platform/types.h"

i32 api_about(const char *args);
i32 api_help(const char *args);
i32 api_ping(const char *args);
i32 api_reboot(const char *args);
i32 api_reset(const char *args);

// Wrapper for `api_reboot()` to bring function signature inline with GET and SET commands (for external callage).
i32 api_misc_reboot(const char *in, char **out);
