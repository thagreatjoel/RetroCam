/*
 * ============================================================
 *  menu.h — boot, loader, tiles, popup, alert, wifi, network,
 *           scan-qr, qr-confirm
 * ============================================================
 */

#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_GFX.h>

#include "types.h"

#include "src/image_passport_happy1_bits.h"
#include "src/Org_01.h"
#include "src/Petme8x8.h"
#include "src/Picopixel.h"
#include "src/icons8_loading.h"

#include "src/image_Ble_connected_bits.h"
#include "src/image_download__2__bits.h"
#include "src/image_External_ant_1_bits.h"
#include "src/image_folder_file_bits.h"
#include "src/image_menu_settings_gear_bits.h"
#include "src/image_usb_cable_connected_bits.h"
#include "src/image_SDQuestion_bits.h"
#include "src/image_Alert_bits.h"
#include "src/image_download__1__bits.h"

#include "src/image_download_bits.h"
#include "src/image_file_search_bits.h"

// =====================================================
// MAIN MENU TILES
// =====================================================
struct Tile {
    int x, y;
    int iconX, iconY;
    int textX, textY;
    const char* label;
    const unsigned char* icon;
    int iconW, iconH;
};

static const Tile TILES[6] = {
    { 10, 54,  21, 56,  16, 76, "Camera",   image_download__2__bits, 21, 21 },
    { 58, 54,  69, 61,  72, 76, "Wifi",     image_External_ant_1_bits, 19, 11 },
    {106, 54, 119, 58, 118, 76, "Files",    image_folder_file_bits, 18, 16 },
    { 10, 88,  24, 92,  26,110, "BT",       image_Ble_connected_bits, 15, 15 },
    { 58, 88,  72, 90,  71,110, "HID",      image_usb_cable_connected_bits, 16, 16 },
    {106, 88, 120, 92, 108,110, "Settings", image_menu_settings_gear_bits, 16, 16 },
};

#define TILE_W 43
#define TILE_H 31

extern TFT_eSPI tft;
extern TFT_eSprite clockSprite;

void clock_redraw(TFT_eSPI* tft, TFT_eSprite* spr);

// =====================================================
// BOOT LOGO
// =====================================================
void menu_drawLogoScreen() {
    tft.startWrite();
    tft.fillScreen(0x0000);
    tft.drawBitmap(57, 30, image_passport_happy1_bits, 46, 49, 0xF206);
    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextColor(0xF206, 0x0000);
    tft.drawCentreString("Welcome", 80, 92, 1);
    tft.endWrite();
}

void menu_drawBlackScreen() {
    tft.startWrite();
    tft.fillScreen(0x0000);
    tft.endWrite();
}

void menu_drawLoader() {
    tft.startWrite();
    tft.fillScreen(0x0000);
    tft.endWrite();
}

void menu_pumpLoader(int* lastFrame) {
    int frame = (millis() / 17) % 61;
    if (frame == *lastFrame) return;
    *lastFrame = frame;
    tft.setBitmapColor(0xF206, 0x0000);
    tft.pushImage(68, 52, 24, 24,
                  icons8_loading_frames[frame],
                  false, nullptr);
}

// =====================================================
// STATUS STRIP (top-left)
// =====================================================
#define STATUS_ICON_X    6
#define STATUS_ICON_Y    3
#define STATUS_TEXT_X    17
#define STATUS_TEXT_Y    5
#define STATUS_BOX_W     60
#define STATUS_BOX_H     18

void menu_drawStatusStrip(bool online, bool apActive) {
    tft.fillRect(0, 0, STATUS_BOX_W, STATUS_BOX_H, 0x0000);

    tft.drawBitmap(STATUS_ICON_X, STATUS_ICON_Y,
                   image_download__1__bits, 8, 8, 0xF206);

    tft.setTextColor(0xF206, 0x0000);
    tft.setTextSize(1);
    tft.setFreeFont(&Picopixel);

    if (online) {
        tft.drawString("ONLINE", STATUS_TEXT_X, STATUS_TEXT_Y);
    } else if (apActive) {
        tft.drawString("AP", STATUS_TEXT_X, STATUS_TEXT_Y);
    } else {
        tft.drawString("OFFLINE", STATUS_TEXT_X, STATUS_TEXT_Y);
        tft.drawLine(6, 11, 13, 20, 0xF206);
    }
}

void menu_statusPatch(bool online, bool apActive) {
    tft.startWrite();
    menu_drawStatusStrip(online, apActive);
    tft.endWrite();
}

// =====================================================
// MAIN MENU TILE DRAW
// =====================================================
static void menu_drawTileOnly(TFT_eSPI& t, int idx, bool selected) {
    const Tile& tile = TILES[idx];

    if (selected) {
        t.fillRect(tile.x + 1, tile.y + 1, TILE_W - 2, TILE_H - 2, 0xF206);
        t.drawBitmap(tile.iconX, tile.iconY, tile.icon, tile.iconW, tile.iconH, 0x0000);
        t.setTextColor(0x0000, 0xF206);
        t.setTextSize(1);
        t.setFreeFont(&Org_01);
        t.drawString(tile.label, tile.textX, tile.textY);
        t.drawRect(tile.x, tile.y, TILE_W, TILE_H, 0x9144);
    } else {
        t.fillRect(tile.x, tile.y, TILE_W, TILE_H, 0x0000);
        t.drawRect(tile.x, tile.y, TILE_W, TILE_H, 0xF206);
        t.drawBitmap(tile.iconX, tile.iconY, tile.icon, tile.iconW, tile.iconH, 0xF206);
        t.setTextColor(0xF206, 0x0000);
        t.setTextSize(1);
        t.setFreeFont(&Org_01);
        t.drawString(tile.label, tile.textX, tile.textY);
    }
}

void menu_drawFull(int menuTile, bool online, bool apActive) {
    tft.startWrite();
    tft.fillScreen(0x0000);
    menu_drawStatusStrip(online, apActive);
    for (int i = 0; i < 6; i++) menu_drawTileOnly(tft, i, i == menuTile);
    tft.endWrite();
    clock_redraw(&tft, &clockSprite);
}

void menu_redrawPatch(int oldTile, int newTile) {
    tft.startWrite();
    menu_drawTileOnly(tft, oldTile, false);
    menu_drawTileOnly(tft, newTile, true);
    tft.endWrite();
}

// =====================================================
// WIFI OPTIONS
// =====================================================
#define WIFI_OPT_X_L   8
#define WIFI_OPT_X_R   83
#define WIFI_OPT_Y     39
#define WIFI_OPT_W     69
#define WIFI_OPT_H     74

static void menu_wifiDrawTile(int idx, bool selected) {
    int x = (idx == 0) ? WIFI_OPT_X_L : WIFI_OPT_X_R;
    const char* label = (idx == 0) ? "NETWORK" : "SCAN";
    const unsigned char* icon = (idx == 0) ? image_download_bits
                                            : image_file_search_bits;
    int iw = (idx == 0) ? 21 : 15;
    int ih = (idx == 0) ? 21 : 16;
    int ix = x + 25;
    int iy = (idx == 0) ? (WIFI_OPT_Y + 17) : (WIFI_OPT_Y + 20);
    int tx = (idx == 0) ? (x + 6)  : (x + 17);
    int ty = WIFI_OPT_Y + 47;

    uint16_t fg, bg;
    if (selected) {
        tft.fillRect(x, WIFI_OPT_Y, WIFI_OPT_W, WIFI_OPT_H, 0xF206);
        tft.drawRect(x, WIFI_OPT_Y, WIFI_OPT_W, WIFI_OPT_H, 0x91C6);
        fg = 0x0000; bg = 0xF206;
    } else {
        tft.fillRect(x, WIFI_OPT_Y, WIFI_OPT_W, WIFI_OPT_H, 0x0000);
        tft.drawRect(x, WIFI_OPT_Y, WIFI_OPT_W, WIFI_OPT_H, 0xF206);
        fg = 0xF206; bg = 0x0000;
    }

    tft.drawBitmap(ix, iy, icon, iw, ih, fg);
    tft.setTextColor(fg, bg);
    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.drawString(label, tx, ty);
}

void menu_drawWifiOptions(int selected) {
    tft.startWrite();
    tft.fillScreen(0x0000);

    tft.setTextColor(0xF206, 0x0000);
    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.drawString("WIFI OPTIONS", 32, 20);

    menu_wifiDrawTile(0, selected == 0);
    menu_wifiDrawTile(1, selected == 1);

    tft.endWrite();
}

void menu_wifiOptionsRedraw(int oldSel, int newSel) {
    tft.startWrite();
    if (oldSel != newSel) {
        menu_wifiDrawTile(oldSel, false);
        menu_wifiDrawTile(newSel, true);
    }
    tft.endWrite();
}

// =====================================================
// NETWORK SUBMENU
// =====================================================
extern bool        g_wifiEnabled;
extern const char* g_wifiModeStr;

#define NET_ROW_X      10
#define NET_ROW_W      141
#define NET_ROW_H      18
#define NET_ROW_Y0     39
#define NET_ROW_STEP   20
#define NET_ROW_COUNT  4

static void network_drawRow(int idx, bool selected) {
    int y = NET_ROW_Y0 + idx * NET_ROW_STEP;

    if (selected) {
        tft.fillRect(NET_ROW_X, y, NET_ROW_W, NET_ROW_H, 0xF206);
        tft.drawRect(NET_ROW_X, y, NET_ROW_W, NET_ROW_H, 0x9144);
        tft.setTextColor(0x0000, 0xF206);
    } else {
        tft.fillRect(NET_ROW_X, y, NET_ROW_W, NET_ROW_H, 0x0000);
        tft.drawRect(NET_ROW_X, y, NET_ROW_W, NET_ROW_H, 0xF206);
        tft.setTextColor(0xF206, 0x0000);
    }

    const char* label = "";
    const char* value = "";
    switch (idx) {
        case 0: label = "WIFI";    value = g_wifiEnabled ? "ON" : "OFF"; break;
        case 1: label = "Connect"; value = ""; break;
        case 2: label = "Mode";    value = g_wifiModeStr; break;
        case 3: label = "TEST";    value = ""; break;
    }

    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.drawString(label, NET_ROW_X + 4, y + 5);

    int px = NET_ROW_X + NET_ROW_W - 35;
    int py = y + 3;
    if (idx == 0 || idx == 2) {
        tft.drawRect(px, py, 31, 12, selected ? 0x0000 : 0xF206);
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setFreeFont(&Picopixel);
        int vw = tft.textWidth(value);
        tft.drawString(value, px + (31 - vw) / 2, py + 3);
        tft.setFreeFont(&Petme8x8);
    } else {
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setFreeFont(&Picopixel);
        tft.drawString(">", NET_ROW_X + NET_ROW_W - 14, y + 5);
        tft.setFreeFont(&Petme8x8);
    }
}

void menu_drawNetwork(int selected) {
    tft.startWrite();
    tft.fillScreen(0x0000);

    tft.setTextColor(0xF206, 0x0000);
    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.drawCentreString("Network", 80, 20, 1);

    for (int i = 0; i < NET_ROW_COUNT; i++) {
        network_drawRow(i, i == selected);
    }

    tft.endWrite();
}

void menu_networkPatch(int oldSel, int newSel) {
    tft.startWrite();
    network_drawRow(oldSel, false);
    network_drawRow(newSel, true);
    tft.endWrite();
}

extern int g_netSelected;
void menu_networkRefreshRow(int idx) {
    tft.startWrite();
    network_drawRow(idx, idx == g_netSelected);
    tft.endWrite();
}

// =====================================================
// SCAN QR — corner tick overlay (from Lopaka export)
// =====================================================
void menu_drawQrFrame() {
    const uint16_t C = 0xF206;

    // Vertical ticks
    tft.drawFastVLine(41,  33, 15, C);   // top-left
    tft.drawFastVLine(119, 33, 15, C);   // top-right
    tft.drawFastVLine(41,  82, 15, C);   // bottom-left
    tft.drawFastVLine(119, 82, 15, C);   // bottom-right

    // Horizontal ticks
    tft.drawFastHLine(41,  32, 16, C);   // top-left
    tft.drawFastHLine(104, 32, 16, C);   // top-right
    tft.drawFastHLine(41,  96, 16, C);   // bottom-left
    tft.drawFastHLine(104, 96, 16, C);   // bottom-right
}

void menu_drawScanQr() {
    // Screen is already filled with camera frames by the caller.
    // We just overlay the ticks + a small "SCAN QR" label.
    tft.startWrite();
    menu_drawQrFrame();

    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.setTextColor(0xF206, 0x0000);
    tft.drawCentreString("SCAN QR", 80, 8, 1);

    tft.endWrite();
}

// =====================================================
// QR CONFIRM — SSID/password + Yes/No
// =====================================================
void menu_drawQrConfirm(const char* ssid, const char* pass, bool yesSelected) {
    tft.startWrite();
    tft.fillScreen(0x0000);

    // Title
    tft.setTextColor(0xF206, 0x0000);
    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.drawCentreString("JOIN THIS NETWORK?", 80, 12, 1);

    // SSID
    tft.setTextColor(0xFFFF, 0x0000);
    tft.drawString("SSID:", 8, 38);
    tft.setTextColor(0xF206, 0x0000);
    tft.drawString(ssid, 8, 50);

    // Password
    tft.setTextColor(0xFFFF, 0x0000);
    tft.drawString("PASS:", 8, 66);
    tft.setTextColor(0xF206, 0x0000);
    tft.drawString(pass, 8, 78);

    // Buttons — Yes at x=15, No at x=99 (reuse existing popup coords)
    auto drawBtn = [](int x, int y, const char* label, bool highlighted) {
        if (highlighted) {
            tft.fillRect(x, y, 44, 20, 0xF206);
            tft.drawRect(x, y, 44, 20, 0xF206);
            tft.setTextColor(0x0000, 0xF206);
        } else {
            tft.fillRect(x, y, 44, 20, 0x0000);
            tft.drawRect(x, y, 44, 20, 0xF206);
            tft.setTextColor(0xF206, 0x0000);
        }
        tft.setTextSize(1);
        tft.setFreeFont(&Petme8x8);
        tft.setCursor(x + 13, y + 7);
        tft.print(label);
    };

    drawBtn(15, 104, "Yes",  yesSelected);
    drawBtn(99, 104, "No",  !yesSelected);

    tft.endWrite();
}

void menu_qrConfirmPatch(bool yesSelected) {
    auto drawBtn = [](int x, int y, const char* label, bool highlighted) {
        if (highlighted) {
            tft.fillRect(x, y, 44, 20, 0xF206);
            tft.drawRect(x, y, 44, 20, 0xF206);
            tft.setTextColor(0x0000, 0xF206);
        } else {
            tft.fillRect(x, y, 44, 20, 0x0000);
            tft.drawRect(x, y, 44, 20, 0xF206);
            tft.setTextColor(0xF206, 0x0000);
        }
        tft.setTextSize(1);
        tft.setFreeFont(&Petme8x8);
        tft.setCursor(x + 13, y + 7);
        tft.print(label);
    };
    tft.startWrite();
    drawBtn(15, 104, "Yes",  yesSelected);
    drawBtn(99, 104, "No",  !yesSelected);
    tft.endWrite();
}

// =====================================================
// POPUP
// =====================================================
static void menu_drawPopupButton(TFT_eSPI& t, int x, int y,
                                 const char* label, bool highlighted) {
    if (highlighted) {
        t.fillRect(x, y, 44, 20, 0xF206);
        t.drawRect(x, y, 44, 20, 0xF206);
        t.setTextColor(0xFFFF, 0xF206);
    } else {
        t.fillRect(x, y, 44, 20, 0x0000);
        t.drawRect(x, y, 44, 20, 0xF206);
        t.setTextColor(0xF206, 0x0000);
    }
    t.setTextFont(1);
    t.setTextSize(1);
    t.setCursor(x + 13, y + 7);
    t.print(label);
}

void menu_drawPopupFull(bool popupYes) {
    tft.startWrite();
    tft.fillScreen(0x0000);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(0xF206, 0x0000);
    tft.setCursor(15, 38); tft.print("Do you want to turn on");
    tft.setCursor(28, 48); tft.print("view live preview?");
    menu_drawPopupButton(tft, 15, 80, "Yes", popupYes);
    menu_drawPopupButton(tft, 99, 80, "No",  !popupYes);
    tft.endWrite();
}

void menu_drawPopupPatch(bool popupYes) {
    tft.startWrite();
    menu_drawPopupButton(tft, 15, 80, "Yes", popupYes);
    menu_drawPopupButton(tft, 99, 80, "No",  !popupYes);
    tft.endWrite();
}

// =====================================================
// SD MISSING
// =====================================================
void menu_drawSdMissing() {
    tft.startWrite();
    tft.fillScreen(0x0000);
    tft.drawBitmap(63, 30, image_SDQuestion_bits, 35, 43, 0xF206);
    tft.setTextColor(0xF206, 0x0000);
    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.drawString("SDcard Not Found!", 13, 81);
    tft.endWrite();
}

// =====================================================
// ALERT
// =====================================================
void menu_drawAlert(const char* errText) {
    tft.startWrite();
    tft.fillScreen(0x0000);
    tft.drawRect(24, 40, 110, 49, 0xF206);
    tft.drawRect(25, 39, 108, 51, 0xF206);
    tft.drawBitmap(66, 44, image_Alert_bits, 28, 24, 0xF206);
    tft.setTextColor(0xF206, 0x0000);
    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.drawCentreString(errText, 80, 73, 1);
    tft.endWrite();
}

// =====================================================
// NOTIFICATION
// =====================================================
void menu_showNotification(const char* l1, const char* l2,
                           uint16_t color, uint32_t holdMs,
                           Mode returnTo = MODE_MENU,
                           int returnTile = -1) {
    (void)holdMs; (void)returnTo; (void)returnTile;

    tft.startWrite();
    tft.fillScreen(0x0000);
    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextColor(0xF206, 0x0000);
    tft.drawCentreString("DuckyMini", 80, 20, 1);
    tft.setTextColor(color, 0x0000);
    tft.setTextSize(2);
    tft.drawCentreString(l1, 80, 70, 1);
    if (l2) {
        tft.setTextSize(1);
        tft.drawCentreString(l2, 80, 95, 1);
    }
    tft.endWrite();
}

// =====================================================
// DIAGNOSTICS
// =====================================================
void menu_drawDiag(bool espConnected, bool sdPresent) {
    tft.startWrite();
    tft.fillScreen(0x0000);
    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextColor(0xF206, 0x0000);
    tft.drawCentreString("DuckyMini", 80, 15, 1);
    tft.setTextSize(1);
    tft.setTextColor(0xFFFF, 0x0000);
    char buf[40];
    int y = 50;
    snprintf(buf, sizeof(buf), "Link: %s", espConnected ? "up" : "down");
    tft.drawString(buf, 8, y); y += 12;
    snprintf(buf, sizeof(buf), "SD:   %s", sdPresent ? "yes" : "no");
    tft.drawString(buf, 8, y); y += 12;
    snprintf(buf, sizeof(buf), "Heap: %u K", ESP.getFreeHeap() / 1024);
    tft.drawString(buf, 8, y); y += 12;
    snprintf(buf, sizeof(buf), "Uptime: %lu s", millis() / 1000);
    tft.drawString(buf, 8, y); y += 12;
    snprintf(buf, sizeof(buf), "Reset:  %d", esp_reset_reason());
    tft.drawString(buf, 8, y);
    tft.endWrite();
}