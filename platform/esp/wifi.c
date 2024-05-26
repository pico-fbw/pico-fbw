/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

// TODO: implement captive portal for esp
// https://github.com/espressif/esp-idf/tree/master/examples/protocols/http_server/captive_portal

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "lib/mimetype.h"

#include "platform/flash.h"
#include "platform/helpers.h"
#include "platform/types.h"

#include "sys/api/cmds/GET/get_config.h"
#include "sys/api/cmds/GET/get_info.h"
#include "sys/api/cmds/SET/set_config.h"
#include "sys/api/cmds/SET/set_flightplan.h"

#include "platform/wifi.h"

#define WIFI_CHANNEL 1 // See https://en.wikipedia.org/wiki/List_of_WLAN_channels
#define WIFI_MAX_CONNECTIONS 4

#define CHUNK_XFER_SIZE 1024

#if PLATFORM_SUPPORTS_WIFI

httpd_handle_t server = NULL;

/**
 * Converts an API response code to an HTTP status code.
 * @param res the API response code
 * @return the HTTP status code (as a string)
 */
static const char *api_res_to_http_status(i32 res) {
    switch (res) {
        case -1: // -1 is an alternate API code which more or less means the same as 200
        case 200:
            return "200 OK";
        case 202:
            return "202 Accepted";
        case 204:
            return "204 No Content";
        case 400:
            return "400 Bad Request";
        case 403:
            return "403 Forbidden";
        case 404:
            return "404 Not Found";
        case 409:
            return "409 Conflict";
        case 500:
        default:
            return "500 Internal Server Error";
    }
}

/**
 * Gets the body of a request and stores it in a buffer.
 * @param req the request
 * @param response the buffer to store the body in
 * @return ESP_OK if successful, an ESP_ERR otherwise
 * @note The buffer will be allocated by this function and must be freed by the caller.
 */
static esp_err_t get_body(httpd_req_t *req, char **response) {
    // First check if the request has a body
    if (req->content_len == 0) {
        *response = NULL;
        return ESP_OK;
    }
    // Allocate a buffer and read in the request body
    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }
    u32 received = 0;
    do {
        i32 len = httpd_req_recv(req, buf + received, req->content_len);
        if (len < 0) {
            httpd_resp_send_500(req);
            free(buf);
            return ESP_FAIL;
        }
        received += len;
    } while (received < req->content_len);
    buf[req->content_len] = '\0';
    *response = buf;
    return ESP_OK;
}

// GET handlers

static esp_err_t handle_api_v1_get_config(httpd_req_t *req) {
    // Get the request body
    char *in = NULL;
    esp_err_t err = get_body(req, &in);
    if (err != ESP_OK)
        return err;
    char *out = NULL;
    // Perform the relavent API call and obtain its output
    i32 res = api_handle_get_config(in, &out);
    // Set the status of the HTTP response to the status of the API response
    httpd_resp_set_status(req, api_res_to_http_status(res));
    if (!out) {
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }
    free(in);
    // Set the content type of the HTTP response to JSON and send the output
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, out);
    free(out);
    return res < 500 ? ESP_OK : ESP_FAIL;
}

static esp_err_t handle_api_v1_get_info(httpd_req_t *req) {
    char *out = NULL;
    i32 res = api_handle_get_info(&out);
    httpd_resp_set_status(req, api_res_to_http_status(res));
    if (!out) {
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, out);
    free(out);
    return res < 500 ? ESP_OK : ESP_FAIL;
}

// SET handlers

static esp_err_t handle_api_v1_set_config(httpd_req_t *req) {
    char *in = NULL;
    esp_err_t err = get_body(req, &in);
    if (err != ESP_OK)
        return err;
    i32 res = api_handle_set_config(in);
    httpd_resp_set_status(req, api_res_to_http_status(res));
    free(in);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, "{}");
    return res < 500 ? ESP_OK : ESP_FAIL;
}

static esp_err_t handle_api_v1_set_flightplan(httpd_req_t *req) {
    char *in = NULL;
    esp_err_t err = get_body(req, &in);
    if (err != ESP_OK)
        return err;
    char *out = NULL;
    i32 res = api_handle_set_flightplan(in, &out);
    httpd_resp_set_status(req, api_res_to_http_status(res));
    if (!out) {
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }
    free(in);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, out);
    free(out);
    return res < 500 ? ESP_OK : ESP_FAIL;
}

// MISC handlers

static esp_err_t handle_api_v1_ping(httpd_req_t *req) {
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, "{}");
    return ESP_OK;
}

// Fetches the content requested by a GET request from littlefs and responds with the content.
// Will be called by the HTTP server when a GET request is received.
static esp_err_t handle_common_get(httpd_req_t *req) {
    // Allocate a buffer to store the path to the file in the filesystem
    // Ensure the buffer is large enough to store anything that could be appended to the path
    size_t pathSize = strlen(req->uri) + sizeof("/www") + sizeof("index.html") + sizeof(".gz");
    char *path = malloc(pathSize);
    if (!path) {
        httpd_resp_send_500(req);
        return ESP_ERR_NO_MEM;
    }
    strcpy(path, "/www"); // Prepend "/www" to the path since web assets are stored in "/www/..." in the filesystem
    // Append the URI to the path
    // If the request is for the root path, append "index.html"
    if (req->uri[strlen(req->uri) - 1] == '/')
        strlcat(path, "/index.html", pathSize);
    else
        strlcat(path, req->uri, pathSize);

    // Check littlefs for the file, both non-gzipped and gzipped variants
    bool gzipped = false;
    lfs_file_t file;
    i32 err;
    for (u32 i = 0; i < 2; i++) {
        err = lfs_file_open(&wwwfs, &file, path, LFS_O_RDONLY);
        if (err == LFS_ERR_NOENT && !gzipped) {
            // File was not found, try once more with the .gz extension
            strcat(path, ".gz");
            gzipped = true;
            continue;
        }
        break;
    }
    if (err != LFS_ERR_OK) {
        httpd_resp_send_404(req);
        free(path);
        return ESP_ERR_NOT_FOUND;
    }

    // Set the content type and encoding headers
    if (gzipped) {
        // Remove the .gz extension to get the correct content type
        // Note that this way of removing the extension does destroy the full path, but it's not used at all later
        path[strlen(path) - 3] = '\0';
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }
    httpd_resp_set_type(req, get_content_type(path));

    // Send the file in chunks
    i32 bytesRead = 0;
    do {
        char chunk[CHUNK_XFER_SIZE];
        bytesRead = lfs_file_read(&wwwfs, &file, chunk, sizeof(chunk));
        if (bytesRead == 0)
            break;
        if (bytesRead < 0 || httpd_resp_send_chunk(req, chunk, bytesRead) != ESP_OK) {
            httpd_resp_sendstr_chunk(req, NULL);
            httpd_resp_send_500(req);
            lfs_file_close(&wwwfs, &file);
            free(path);
            return ESP_FAIL;
        }
    } while (bytesRead > 0);
    httpd_resp_send_chunk(req, NULL, 0);
    lfs_file_close(&wwwfs, &file);
    free(path);
    return ESP_OK;
}

bool wifi_setup(const char *ssid, const char *pass) {
    if (!ssid || strlen(ssid) < WIFI_SSID_MIN_LEN || strlen(ssid) > WIFI_SSID_MAX_LEN)
        return false;
    if (pass && (strlen(pass) < WIFI_PASS_MIN_LEN || strlen(pass) > WIFI_PASS_MAX_LEN))
        return false;

    // Initialize underlying network stack
    // NVS is initialized in sys_boot_begin()
    if (esp_netif_init() != ESP_OK)
        return false;
    if (esp_event_loop_create_default() != ESP_OK)
        return false;
    esp_netif_create_default_wifi_ap();

    // Initialize wifi access point
    wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&initConfig) != ESP_OK)
        return false;
    // clang-format off
    wifi_config_t wifiConfig = {
        .ap = {
            // ssid and password are copied below, since thety are stored as arrays and not pointers
            .ssid_len = strlen(ssid),
            .channel = WIFI_CHANNEL,
            .max_connection = WIFI_MAX_CONNECTIONS,
            .authmode = pass ? WIFI_AUTH_WPA2_WPA3_PSK : WIFI_AUTH_OPEN,
            .pmf_cfg = {
                .required = true,
            },
        },
    };
    // clang-format on
    strncpy((char *)wifiConfig.ap.ssid, ssid, sizeof(wifiConfig.ap.ssid));
    if (pass)
        strncpy((char *)wifiConfig.ap.password, pass, sizeof(wifiConfig.ap.password));
    if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK)
        return false;
    if (esp_wifi_set_config(WIFI_IF_AP, &wifiConfig) != ESP_OK)
        return false;
    if (esp_wifi_start() != ESP_OK)
        return false;

    // Initialize the HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    if (httpd_start(&server, &config) != ESP_OK)
        return false;

    // API handlers
    httpd_uri_t apiV1GetConfigURIGet = {
        .uri = "/api/v1/get/config",
        .method = HTTP_GET,
        .handler = handle_api_v1_get_config,
    };
    httpd_uri_t apiV1GetConfigURIPost = {
        .uri = "/api/v1/get/config",
        .method = HTTP_POST,
        .handler = handle_api_v1_get_config,
    };
    // get/config supports both GET and POST
    httpd_register_uri_handler(server, &apiV1GetConfigURIGet);
    httpd_register_uri_handler(server, &apiV1GetConfigURIPost);
    httpd_uri_t apiV1GetInfoURI = {
        .uri = "/api/v1/get/info",
        .method = HTTP_GET,
        .handler = handle_api_v1_get_info,
    };
    httpd_register_uri_handler(server, &apiV1GetInfoURI);
    httpd_uri_t apiV1SetConfigURI = {
        .uri = "/api/v1/set/config",
        .method = HTTP_POST,
        .handler = handle_api_v1_set_config,
    };
    httpd_register_uri_handler(server, &apiV1SetConfigURI);
    httpd_uri_t apiV1SetFlightplanURI = {
        .uri = "/api/v1/set/flightplan",
        .method = HTTP_POST,
        .handler = handle_api_v1_set_flightplan,
    };
    httpd_register_uri_handler(server, &apiV1SetFlightplanURI);
    httpd_uri_t apiV1PingURI = {
        .uri = "/api/v1/ping",
        .method = HTTP_GET,
        .handler = handle_api_v1_ping,
    };
    httpd_register_uri_handler(server, &apiV1PingURI);

    // Common GET handler (for serving files)
    httpd_uri_t commonGETURI = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = handle_common_get,
    };
    httpd_register_uri_handler(server, &commonGETURI);

    return true;
}

void wifi_periodic() {
    return; // esp wifi handles events in background through tasks
}

bool wifi_disable() {
    if (esp_wifi_stop() != ESP_OK)
        return false;
    if (esp_wifi_deinit() != ESP_OK)
        return false;
    if (httpd_stop(server) != ESP_OK)
        return false;
    if (esp_event_loop_delete_default() != ESP_OK)
        return false;
    return esp_netif_deinit() == ESP_OK;
}

#endif // PLATFORM_SUPPORTS_WIFI
