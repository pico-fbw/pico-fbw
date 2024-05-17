/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "platform/flash.h"
#include "platform/helpers.h"
#include "resources/mimetype.h"

#include "platform/wifi.h"

#define WIFI_CHANNEL 1 // See https://en.wikipedia.org/wiki/List_of_WLAN_channels
#define WIFI_MAX_CONNECTIONS 4

#define MSG_FILE_FAIL "Failed to read file!"
#define MSG_FILE_NOT_FOUND "File not found!"
#define CHUNK_XFER_SIZE 1024

#if PLATFORM_SUPPORTS_WIFI

static esp_err_t handle_api_v1_ping(httpd_req_t *req) {
    httpd_resp_sendstr(req, "{}");
    return ESP_OK;
}

// Fetches the content requested by a GET request from littlefs and responds with the content.
// Will be called by the HTTP server when a GET request is received.
static esp_err_t handle_get(httpd_req_t *req) {
    // Allocate a buffer to store the path to the file in the filesystem
    // Ensure the buffer is large enough to store anything that could be appended to the path
    size_t pathSize = strlen(req->uri) + sizeof("/www") + sizeof("index.html") + sizeof(".gz");
    char *path = malloc(pathSize);
    if (!path) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, MSG_FILE_FAIL);
        return ESP_ERR_NO_MEM;
    }
    strcpy(path, "/www"); // Prepend "/www" to the path since web assets are stored in "/www/..." in the wwwfs
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
    if (err == LFS_ERR_NOENT) {
        // Redirect to / if the file wasn't found (captive portal)
        httpd_resp_set_status(req, "302 Redirect");
        httpd_resp_set_hdr(req, "Location", "/");
        free(path);
        return ESP_OK;
    }
    if (err != LFS_ERR_OK) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, MSG_FILE_NOT_FOUND);
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
        if (bytesRead < 0 || httpd_resp_send_chunk(req, chunk, bytesRead) != ESP_OK) {
            httpd_resp_sendstr_chunk(req, NULL);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, MSG_FILE_FAIL);
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
    wifi_config_t wifi_config = {
        .ap =
            {
                // ssid and password are copied below, since thety are stored as arrays and not pointers
                .ssid_len = strlen(ssid),
                .channel = WIFI_CHANNEL,
                .max_connection = WIFI_MAX_CONNECTIONS,
                .authmode = pass ? WIFI_AUTH_WPA2_WPA3_PSK : WIFI_AUTH_OPEN,
                .pmf_cfg =
                    {
                        .required = true,
                    },
            },
    };
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    if (pass)
        strncpy((char *)wifi_config.ap.password, pass, sizeof(wifi_config.ap.password));
    if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK)
        return false;
    if (esp_wifi_set_config(WIFI_IF_AP, &wifi_config) != ESP_OK)
        return false;
    if (esp_wifi_start() != ESP_OK)
        return false;

    // Initialize the HTTP server
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    if (httpd_start(&server, &config) != ESP_OK)
        return false;

    // API (v1)
    httpd_uri_t apiV1PingURI = {
        .uri = "/api/v1/ping",
        .method = HTTP_GET,
        .handler = handle_api_v1_ping,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &apiV1PingURI);

    // Handle common GET requests
    httpd_uri_t commonGETURI = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = handle_get,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &commonGETURI);

    return true;
}

void wifi_periodic() {
    return; // esp wifi handles events in background through tasks
}

#endif // PLATFORM_SUPPORTS_WIFI
