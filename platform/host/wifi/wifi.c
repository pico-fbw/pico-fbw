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

#include "mongoose.h"

#include "sys/api/api.h"
#include "sys/api/cmds/GET/get_config.h"
#include "sys/api/cmds/GET/get_info.h"
#include "sys/api/cmds/GET/get_logs.h"
#include "sys/api/cmds/SET/set_config.h"
#include "sys/api/cmds/SET/set_flightplan.h"

#define DOCUMENT_ROOT "./www/www" // Root directory for the web server
#define POLL_PERIOD_MS 100 // Rate at which the web server is polled

// clang-format on

static struct mg_mgr mgr;

static void handle_api_v1_request(struct mg_connection *c, struct mg_http_message *hm, api_handler handler) {
    char *out = NULL;
    i32 res = 200;
    if (handler)
        res = handler(hm->body.len > 0 ? hm->body.buf : NULL, &out);
    mg_http_reply(c, res, "Content-Type: application/json\r\n", out ? out : "{}");
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev != MG_EV_HTTP_MSG)
        return;
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (mg_match(hm->uri, mg_str("/api/v1/get/config"), NULL)) {
        handle_api_v1_request(c, hm, api_handle_get_config);
    } else if (mg_match(hm->uri, mg_str("/api/v1/get/info"), NULL)) {
        handle_api_v1_request(c, hm, api_handle_get_info);
    } else if (mg_match(hm->uri, mg_str("/api/v1/get/logs"), NULL)) {
        handle_api_v1_request(c, hm, api_handle_get_logs);
    } else if (mg_match(hm->uri, mg_str("/api/v1/set/config"), NULL)) {
        handle_api_v1_request(c, hm, api_handle_set_config);
    } else if (mg_match(hm->uri, mg_str("/api/v1/set/flightplan"), NULL)) {
        handle_api_v1_request(c, hm, api_handle_set_flightplan);
    } else if (mg_match(hm->uri, mg_str("/api/v1/ping"), NULL)) {
        handle_api_v1_request(c, hm, NULL);
    } else {
        struct mg_http_serve_opts opts = {.root_dir = DOCUMENT_ROOT};
        mg_http_serve_dir(c, hm, &opts);
    }
}

bool wifi_setup(const char *ssid, const char *pass) {
    mg_mgr_init(&mgr);
    return mg_http_listen(&mgr, "http://0.0.0.0:5173", ev_handler, NULL) != NULL;
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
