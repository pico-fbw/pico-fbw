/**
 * pico-fbw's host Wi-Fi (but not really Wi-Fi) implementation is curteousy of Mongoose.
 * Check it out at https://github.com/cesanta/mongoose!
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/wifi.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <limits.h>
#include <stdio.h>

#include "mongoose.h"

#include "sys/api/api.h"
#include "sys/api/cmds/GET/get_config.h"
#include "sys/api/cmds/GET/get_flightplan.h"
#include "sys/api/cmds/GET/get_info.h"
#include "sys/api/cmds/GET/get_logs.h"
#include "sys/api/cmds/SET/set_config.h"
#include "sys/api/cmds/SET/set_flightplan.h"

#define DOCUMENT_ROOT "./www/www" // Root directory for the web server
#define POLL_PERIOD_MS 2 // Rate at which the web server is polled

// clang-format on

static struct mg_mgr mgr;

// Handles an API request from an HTTP event.
static void handle_api_v1_request(struct mg_connection *c, struct mg_http_message *hm, api_handler handler) {
    char *out = NULL;
    i32 res = 200;
    if (handler)
        res = handler(hm->body.len > 0 ? hm->body.buf : NULL, &out);
    mg_http_reply(c, res, "Content-Type: application/json\r\n", out ? out : "{}");
}

// Mongoose callback. Called when an HTTP event occurs.
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev != MG_EV_HTTP_MSG)
        return;
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    // Handle API requests
    if (mg_match(hm->uri, mg_str("/api/v1/get/config"), NULL))
        handle_api_v1_request(c, hm, api_handle_get_config);
    else if (mg_match(hm->uri, mg_str("/api/v1/get/flightplan"), NULL))
        handle_api_v1_request(c, hm, api_handle_get_flightplan);
    else if (mg_match(hm->uri, mg_str("/api/v1/get/info"), NULL))
        handle_api_v1_request(c, hm, api_handle_get_info);
    else if (mg_match(hm->uri, mg_str("/api/v1/get/logs"), NULL))
        handle_api_v1_request(c, hm, api_handle_get_logs);
    else if (mg_match(hm->uri, mg_str("/api/v1/set/config"), NULL))
        handle_api_v1_request(c, hm, api_handle_set_config);
    else if (mg_match(hm->uri, mg_str("/api/v1/set/flightplan"), NULL))
        handle_api_v1_request(c, hm, api_handle_set_flightplan);
    else if (mg_match(hm->uri, mg_str("/api/v1/ping"), NULL))
        handle_api_v1_request(c, hm, NULL);
    else {
        // No matching API request, serve static files instead
        // Check if the requested file exists
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s%.*s", DOCUMENT_ROOT, hm->uri.len, hm->uri.buf);
        // Root path should serve index.html
        if (hm->uri.buf[hm->uri.len - 1] == '/')
            strcat(path, "index.html");
        bool found = false;
        for (u32 i = 0; i < 2; i++) {
            FILE *fp = fopen(path, "rb");
            if (fp) {
                fclose(fp);
                found = true;
            } else {
                strcat(path, ".gz"); // Check for a gzipped variant
                continue;
            }
        }
        if (found) {
            // File exists, mongoose can handle this
            struct mg_http_serve_opts opts = {
                .root_dir = DOCUMENT_ROOT,
            };
            mg_http_serve_dir(c, hm, &opts);
        } else
            // File doesn't exist, redirect to root
            mg_http_reply(c, 302, "Location: /\r\n", "");
    }
}

bool wifi_setup(const char *ssid, const char *pass) {
    mg_mgr_init(&mgr);
    if (!mg_http_listen(&mgr, "http://0.0.0.0:5173", ev_handler, NULL))
        return false;
    printf("emulated Wi-Fi access point started at http://localhost:5173\n");
    return true;
    (void)ssid;
    (void)pass;
}

void wifi_periodic() {
    mg_mgr_poll(&mgr, POLL_PERIOD_MS);
}

bool wifi_disable() {
    mg_mgr_free(&mgr);
    return true;
}

#endif // PLATFORM_SUPPORTS_WIFI
