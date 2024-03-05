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

#define FS_PARTITION_LABEL "fs" // Name of the littlefs partition defined in platform/esp/resources/partitions.csv

static const esp_partition_t *partition;
static SemaphoreHandle_t lfs_lock = NULL;

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    assert(partition);
    assert(block < c->block_count);
    assert(off + size <= c->block_size);
    if (esp_partition_read(partition, (block * c->block_size) + off, buffer, size) != ESP_OK)
        return LFS_ERR_IO;
    return LFS_ERR_OK;
}

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    assert(partition);
    assert(block < c->block_count);
    if (esp_partition_write(partition, (block * c->block_size) + off, buffer, size) != ESP_OK)
        return LFS_ERR_IO;
    return LFS_ERR_OK;
}

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {
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
    if (lfs_lock == NULL) {
        static portMUX_TYPE lfs_lock_mux = portMUX_INITIALIZER_UNLOCKED;
        portENTER_CRITICAL(&lfs_lock_mux);
        if (lfs_lock == NULL)
            lfs_lock = xSemaphoreCreateMutex();
        portEXIT_CRITICAL(&lfs_lock_mux);
    }
    if (xSemaphoreTake(lfs_lock, portMAX_DELAY) == pdTRUE)
        return LFS_ERR_OK;
    else
        return LFS_ERR_IO;
    (void)c;
}

static int flash_unlock(const struct lfs_config *c) {
    if (xSemaphoreGive(lfs_lock) == pdTRUE)
        return LFS_ERR_OK;
    else
        return LFS_ERR_IO;
    (void)c;
}

bool flash_setup() {
    // Set partition to the littlefs partition defined in partitions.csv
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LITTLEFS, FS_PARTITION_LABEL);
    if (partition == NULL)
        return false;
    // Auto-detect block count based on partition size
    lfs_cfg.block_count = partition->size / lfs_cfg.block_size;
    if (lfs_cfg.block_count == 0)
        return false;
    return true;
}

struct lfs_config lfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
    .lock = flash_lock,
    .unlock = flash_unlock,
    .read_size = READ_SIZE,
    .prog_size = WRITE_SIZE,
    .block_size = BLOCK_SIZE,
    .block_count = 0, // Will be automatically set based on the partition size in flash_setup()
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_SIZE,
    .block_cycles = BLOCK_CYCLES,
};

lfs_t lfs;
