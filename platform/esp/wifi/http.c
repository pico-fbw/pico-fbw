/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "http.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "esp_event.h"

#include "lib/mimetype.h"

#include "platform/flash.h"
#include "platform/helpers.h"
#include "platform/types.h"

#include "sys/api/api.h"
#include "sys/api/cmds/GET/get_config.h"
#include "sys/api/cmds/GET/get_info.h"
#include "sys/api/cmds/SET/set_config.h"
#include "sys/api/cmds/SET/set_flightplan.h"

#define CHUNK_XFER_SIZE 1024

// clang-format on

/**
 * Gets the body of a request and stores it in a buffer.
 * @param req the request
 * @param response the buffer to store the body in
 * @return ESP_OK if successful, an ESP_ERR otherwise
 * @note The buffer will be allocated by this function and must be freed by the caller.
 */
static esp_err_t get_request_body(httpd_req_t *req, char **response) {
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
    esp_err_t err = get_request_body(req, &in);
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
    esp_err_t err = get_request_body(req, &in);
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
    esp_err_t err = get_request_body(req, &in);
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
        // Redirect the client to the index page; this provides the captive portal behavior
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/");
        // To redirect on ios devices, there must be a response body
        httpd_resp_send(req, "pico-fbw", HTTPD_RESP_USE_STRLEN);
        free(path);
        return ESP_OK;
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

esp_err_t http_server_open(httpd_handle_t *server) {
    httpd_config_t httpdConfig = HTTPD_DEFAULT_CONFIG();
    httpdConfig.uri_match_fn = httpd_uri_match_wildcard;
    httpdConfig.max_open_sockets = 13;
    httpdConfig.lru_purge_enable = true;
    if (httpd_start(&server, &httpdConfig) != ESP_OK)
        return ESP_FAIL;

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

    return ESP_OK;
}

esp_err_t http_server_close(httpd_handle_t server) {
    return httpd_stop(server);
}

#endif // PLATFORM_SUPPORTS_WIFI
