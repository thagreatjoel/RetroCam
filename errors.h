#pragma once

// Mini-side
#define ERR_ESPNOW_SEND      100
#define ERR_PREVIEW_TIMEOUT  101
#define ERR_CAM_NO_REPLY     102
#define ERR_CAMERA_BUSY      103
#define ERR_SD_TIMEOUT       104
#define ERR_CHANNEL_MISMATCH 105

// CAM-side
#define ERR_CAM_INIT         150
#define ERR_SD_MOUNT         151
#define ERR_CAPTURE_FAILED   152
#define ERR_STORAGE_WRITE    153
#define ERR_STORAGE_FULL     154
#define ERR_STREAM_NULL      155

// Helper to format "Error NNN"
static inline void formatErr(char* out, size_t n, int code) {
    snprintf(out, n, "Error %d", code);
}