/**
 * pico-fbw's host Wi-Fi (but not really Wi-Fi) implementation is curteousy of Mongoose.
 * Check it out at https://github.com/cesanta/mongoose
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/wifi.h"

#if PLATFORM_SUPPORTS_WIFI

// clang-format off

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "mongoose.h"

#include "platform/flash.h"

#include "sys/api/api.h"

#define POLL_PERIOD_MS 2 // Rate at which the web server is polled

// clang-format on

static struct mg_mgr mgr;

static int fs_st(const char *path, size_t *size, time_t *mtime) {
    struct lfs_info info;
    int res = lfs_stat(&wwwfs, path, &info);
    if (res < 0) {
        return res;
    }
    if (size) {
        *size = info.size;
    }
    // littlefs doesn't support modification times, so just return the current time
    if (mtime) {
        *mtime = time(NULL);
    }
    return MG_FS_READ | MG_FS_WRITE | (info.type == LFS_TYPE_DIR ? MG_FS_DIR : 0);
    return 0;
}

static void fs_ls(const char *path, void (*fn)(const char *, void *), void *userdata) {
    lfs_dir_t dir;
    struct lfs_info info;
    int res = lfs_dir_open(&wwwfs, &dir, path);
    if (res < 0) {
        return;
    }
    printf("LOG: opened directory %s\n", path);
    while (lfs_dir_read(&wwwfs, &dir, &info) > 0) {
        printf("LOG: found file %s\n", info.name);
        if (strcmp(info.name, ".") != 0 && strcmp(info.name, "..") != 0) {
            printf("LOG: calling function with file %s\n", info.name);
            fn(info.name, userdata);
        }
    }
    lfs_dir_close(&wwwfs, &dir);
}

static void *fs_op(const char *path, int flags) {
    int open_flags = flags == MG_FS_READ ? LFS_O_RDONLY : LFS_O_RDWR | LFS_O_CREAT;
    lfs_file_t *file = (lfs_file_t *)malloc(sizeof(lfs_file_t));
    if (!file) {
        return NULL;
    }
    int res = lfs_file_open(&wwwfs, file, path, open_flags);
    if (res < 0) {
        free(file);
        return NULL;
    }
    return file;
}

static void fs_cl(void *fd) {
    lfs_file_close(&wwwfs, (lfs_file_t *)fd);
    free(fd);
}

static size_t fs_rd(void *fd, void *buf, size_t len) {
    return lfs_file_read(&wwwfs, (lfs_file_t *)fd, buf, len);
}

static size_t fs_wr(void *fd, const void *buf, size_t len) {
    return lfs_file_write(&wwwfs, (lfs_file_t *)fd, buf, len);
}

static size_t fs_sk(void *fd, size_t offset) {
    return lfs_file_seek(&wwwfs, (lfs_file_t *)fd, offset, LFS_SEEK_SET);
}

static bool fs_mv(const char *from, const char *to) {
    return lfs_rename(&wwwfs, from, to) == LFS_ERR_OK;
}

static bool fs_rm(const char *path) {
    return lfs_remove(&wwwfs, path) == LFS_ERR_OK;
}

static bool fs_mkd(const char *path) {
    return lfs_mkdir(&wwwfs, path) == LFS_ERR_OK;
}

// Handles an API request from an HTTP event.
static void handle_api_v1_request(struct mg_connection *c, struct mg_http_message *hm, api_handler handler) {
    char *out = NULL;
    i32 res = 200;
    if (handler) {
        res = handler(hm->body.len > 0 ? hm->body.buf : NULL, &out);
    }
    mg_http_reply(c, res, "Content-Type: application/json\r\n", out ? out : "{}");
}

// Mongoose callback. Called when an HTTP event occurs.
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev != MG_EV_HTTP_MSG) {
        return;
    }
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    // Handle API requests
    if (mg_match(hm->uri, mg_str("/api/v1/get/config"), NULL)) {
        handle_api_v1_request(c, hm, api_get_config);
    } else if (mg_match(hm->uri, mg_str("/api/v1/get/flightplan"), NULL)) {
        handle_api_v1_request(c, hm, api_get_flightplan);
    } else if (mg_match(hm->uri, mg_str("/api/v1/get/info"), NULL)) {
        handle_api_v1_request(c, hm, api_get_info);
    } else if (mg_match(hm->uri, mg_str("/api/v1/get/input"), NULL)) {
        handle_api_v1_request(c, hm, api_get_input);
    } else if (mg_match(hm->uri, mg_str("/api/v1/get/logs"), NULL)) {
        handle_api_v1_request(c, hm, api_get_logs);
    } else if (mg_match(hm->uri, mg_str("/api/v1/get/mode"), NULL)) {
        handle_api_v1_request(c, hm, api_get_mode);
    } else if (mg_match(hm->uri, mg_str("/api/v1/get/sensor"), NULL)) {
        handle_api_v1_request(c, hm, api_get_sensor);
    } else if (mg_match(hm->uri, mg_str("/api/v1/set/bay"), NULL)) {
        handle_api_v1_request(c, hm, api_set_bay);
    } else if (mg_match(hm->uri, mg_str("/api/v1/set/config"), NULL)) {
        handle_api_v1_request(c, hm, api_set_config);
    } else if (mg_match(hm->uri, mg_str("/api/v1/set/flightplan"), NULL)) {
        handle_api_v1_request(c, hm, api_set_flightplan);
    } else if (mg_match(hm->uri, mg_str("/api/v1/set/mode"), NULL)) {
        handle_api_v1_request(c, hm, api_set_mode);
    } else if (mg_match(hm->uri, mg_str("/api/v1/set/target"), NULL)) {
        handle_api_v1_request(c, hm, api_set_target);
    } else if (mg_match(hm->uri, mg_str("/api/v1/set/waypoint"), NULL)) {
        handle_api_v1_request(c, hm, api_set_waypoint);
    } else if (mg_match(hm->uri, mg_str("/api/v1/ping"), NULL)) {
        handle_api_v1_request(c, hm, NULL);
    } else {
        // No matching API request, serve static files instead
        struct mg_http_serve_opts opts = {
            .root_dir = "/www",
            .fs =
                &(struct mg_fs){
                    .st = fs_st,
                    .ls = fs_ls,
                    .op = fs_op,
                    .cl = fs_cl,
                    .rd = fs_rd,
                    .wr = fs_wr,
                    .sk = fs_sk,
                    .mv = fs_mv,
                    .rm = fs_rm,
                    .mkd = fs_mkd,
                },
        };
        mg_http_serve_dir(c, hm, &opts);
    }
}

bool wifi_setup(const char *ssid, const char *pass) {
    mg_mgr_init(&mgr);
    if (!mg_http_listen(&mgr, "http://0.0.0.0:5173", ev_handler, NULL)) {
        return false;
    }
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
