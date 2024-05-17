#pragma once

#include <string.h>
#include "platform/helpers.h"
#include "platform/types.h"

typedef struct MimeEntry {
    const char *endsWith;
    const char *mimeType;
} MimeEntry;

// clang-format off
const MimeEntry mimeTable[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".txt", "text/plain"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".ttf", "application/x-font-ttf"},
    {".otf", "application/x-font-opentype"},
    {".woff", "application/font-woff"},
    {".woff2", "application/font-woff2"},
    {".eot", "application/vnd.ms-fontobject"},
    {".sfnt", "application/font-sfnt"},
    {".xml", "text/xml"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".gz", "application/x-gzip"},
    {".appcache", "text/cache-manifest"},
    {"", "application/octet-stream"}
};
// clang-format on

/**
 * Get the content type of a file based on its path/filename.
 * @param path path to/filename of the file
 * @return the content type of the file
 */
static inline const char *get_content_type(const char *path) {
    for (u32 i = 0; i < count_of(mimeTable); i++) {
        const char *suffix = mimeTable[i].endsWith;
        size_t lenpath = strlen(path);
        size_t lensuffix = strlen(suffix);
        if (lenpath >= lensuffix) {
            const char *check = strstr(path + lenpath - lensuffix, suffix);
            if (check != NULL)
                return mimeTable[i].mimeType;
        }
    }
    return mimeTable[count_of(mimeTable)].mimeType; // Default to octet-stream
}
