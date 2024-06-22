/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

/**
 * Thanks a lot to the contributors of the `esp_littlefs` library for the implementation of littlefs for the ESP32,
 * check it out here: https://github.com/joltwallet/esp_littlefs
 */

#include <assert.h>
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "platform/flash.h"

// FS configuration, littlefs documentation explains these settings in detail (see lib/lfs.h)
#define READ_SIZE 128
#define WRITE_SIZE 128
#define BLOCK_SIZE 4096
#define CACHE_SIZE 512
#define LOOKAHEAD_SIZE 128
#define BLOCK_CYCLES 512

#define LFS_PARTITION_LABEL "lfs" // Name of the littlefs partition defined in platform/esp/resources/partitions.csv
#define WWWFS_PARTITION_LABEL "wwwfs"

static const esp_partition_t *wwwfsPartition, *lfsPartition;
static SemaphoreHandle_t lfsLock = NULL; // Mutex to prevent concurrent access to flash

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    esp_partition_t *partition = (esp_partition_t *)c->context;
    assert(partition);
    assert(block < c->block_count);
    assert(off + size <= c->block_size);
    if (esp_partition_read(partition, (block * c->block_size) + off, buffer, size) != ESP_OK)
        return LFS_ERR_IO;
    return LFS_ERR_OK;
}

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    esp_partition_t *partition = (esp_partition_t *)c->context;
    assert(partition);
    assert(block < c->block_count);
    if (esp_partition_write(partition, (block * c->block_size) + off, buffer, size) != ESP_OK)
        return LFS_ERR_IO;
    return LFS_ERR_OK;
}

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {
    esp_partition_t *partition = (esp_partition_t *)c->context;
    assert(partition);
    assert(block < c->block_count);
    if (esp_partition_erase_range(partition, block * c->block_size, c->block_size) != ESP_OK)
        return LFS_ERR_IO;
    return LFS_ERR_OK;
}

static int flash_sync(const struct lfs_config *c) {
    // No sync required with ESP-IDF
    return LFS_ERR_OK;
    (void)c; // Supress unused parameter warning
}

static int flash_lock(const struct lfs_config *c) {
    if (!lfsLock) {
        static portMUX_TYPE lfsLock_mux = portMUX_INITIALIZER_UNLOCKED;
        portENTER_CRITICAL(&lfsLock_mux);
        if (!lfsLock)
            lfsLock = xSemaphoreCreateMutex();
        portEXIT_CRITICAL(&lfsLock_mux);
    }
    if (xSemaphoreTake(lfsLock, portMAX_DELAY) == pdTRUE)
        return LFS_ERR_OK;
    else
        return LFS_ERR_IO;
    (void)c;
}

static int flash_unlock(const struct lfs_config *c) {
    if (xSemaphoreGive(lfsLock) == pdTRUE)
        return LFS_ERR_OK;
    else
        return LFS_ERR_IO;
    (void)c;
}

bool flash_setup() {
#if PLATFORM_SUPPORTS_WIFI
    // Find partitions defined in partitions.csv
    wwwfsPartition =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LITTLEFS, WWWFS_PARTITION_LABEL);
    if (!wwwfsPartition)
        return false;
    // Pass partition as context so block operations know where to read/write
    wwwfs_cfg.context = (void *)wwwfsPartition;
    // Auto-detect block count based on partition size
    wwwfs_cfg.block_count = wwwfsPartition->size / wwwfs_cfg.block_size;
    if (wwwfs_cfg.block_count <= 0)
        return false;
#endif
    lfsPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LITTLEFS, LFS_PARTITION_LABEL);
    if (!lfsPartition)
        return false;
    lfs_cfg.context = (void *)lfsPartition;
    lfs_cfg.block_count = lfsPartition->size / lfs_cfg.block_size;
    return lfs_cfg.block_count > 0;
}

lfs_t lfs;
struct lfs_config lfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
    .lock = flash_lock,
    .unlock = flash_unlock,
    // context is set in flash_setup()
    .read_size = READ_SIZE,
    .prog_size = WRITE_SIZE,
    .block_size = BLOCK_SIZE,
    // block_count is set in flash_setup()
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_SIZE,
    .block_cycles = BLOCK_CYCLES,
};

#if PLATFORM_SUPPORTS_WIFI
lfs_t wwwfs;
struct lfs_config wwwfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
    .lock = flash_lock,
    .unlock = flash_unlock,
    .read_size = READ_SIZE,
    .prog_size = WRITE_SIZE,
    .block_size = BLOCK_SIZE,
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_SIZE,
    .block_cycles = BLOCK_CYCLES,
};
#endif
