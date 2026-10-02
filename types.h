#pragma once

enum Mode {
    MODE_BOOT,
    MODE_MENU,
    MODE_LOADING,
    MODE_POPUP,
    MODE_PREVIEW,
    MODE_DIAG,
    MODE_NOTIFY,
    MODE_SD_MISSING,
    MODE_ALERT,
    MODE_WIFI_OPTIONS,
    MODE_NETWORK,
    MODE_SCAN_QR,        // live camera + tick overlay
    MODE_QR_CONFIRM      // SSID/pass + Yes/No
};

enum LoadPurpose {
    LOAD_FOR_PREVIEW,
    LOAD_FOR_SD_CHECK,
    LOAD_FOR_LOCAL_SD_MISSING
};