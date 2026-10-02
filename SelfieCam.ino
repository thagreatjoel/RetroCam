/*
 * ============================================================
 *  ESP32-C3 SuperMini — DuckyMini
 *
 *  Connect flow:
 *    Network submenu → Connect → MODE_SCAN_QR
 *    Mini sends SCAN_QR_START to CAM
 *    CAM switches to grayscale, runs QR decode
 *    On decode → CAM sends QR_DATA <ssid> <pass>
 *    Mini → MODE_QR_CONFIRM
 *    Yes → logs + SAVED notify (real WiFi.begin lands later)
 *
 *  Serial test: type   QR MyPhone mypassword123
 *  in the Serial Monitor to fake a scan.
 *
 *  GPIO 3 short → cycle / toggle Yes-No
 *  GPIO 5 short → activate / confirm
 *  GPIO 5 hold  → back one level (500 ms)
 *  LED GPIO 9   → state indicator,  Buttons: zero delay
 * ============================================================
 */

#include <SPI.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
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

#include "menu.h"
#include "espnow_link.h"
#include "ntp.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite clockSprite = TFT_eSprite(&tft);

#define BTN_SCREEN    3
#define BTN_SELECT    5
#define BACKLIGHT_PIN 2

#define VIBE_PIN      1
#define VIBE_MS       40

#define LED_PIN       9
#define LED_ON        HIGH
#define LED_OFF       LOW

#define LONG_PRESS_MS 500

#define AP_SSID       "DuckyMini"
#define AP_PASS       "12345678"

#define FRAME_W        160
#define FRAME_H        120
#define ROW_STRETCH    128
#define MAX_JPEG       16384

uint16_t* streamBuf = nullptr;
uint16_t* tftBuf    = nullptr;

Mode currentMode = MODE_BOOT;
LoadPurpose loadPurpose = LOAD_FOR_PREVIEW;

int  menuTile         = 0;
int  wifiOptSel       = 0;
int  g_netSelected    = 0;

bool        g_wifiEnabled  = false;
const char* g_wifiModeStr  = "AP+STA";

bool g_wifiOnline   = false;
bool g_wifiApActive = false;

// QR scan state
String   qrSsid        = "";
String   qrPass        = "";
bool     qrYesSelected = false;
uint32_t qrScanStartMs = 0;

bool popupYes         = false;
uint32_t bootStart         = 0;
int      bootPhase         = 0;
uint32_t lastFrameTime     = 0;
uint32_t framesThisSec     = 0;
uint32_t fps               = 0;
uint32_t lastFpsCalc       = 0;
uint32_t notifyUntil       = 0;
uint32_t diagRefresh       = 0;
Mode     notifyReturnMode  = MODE_MENU;
int      notifyReturnTile  = 0;

bool sdPresent       = false;
bool sdCheckPending  = false;
int  sdAttempts      = 0;
uint32_t sdCheckSent = 0;

int lastLoaderFrame = -1;

uint32_t alertShowUntil = 0;
Mode     alertReturnMode = MODE_MENU;
int      alertReturnTile = 0;

// ---- GPIO 3 ----
bool btn3LastReading = HIGH;
uint32_t btn3PressStart = 0;
bool btn3Held = false;
bool btn3LongPressTriggered = false;

// ---- GPIO 5 ----
bool btn5LastReading = HIGH;
uint32_t btn5PressStart = 0;
bool btn5Held = false;
bool btn5LongPressTriggered = false;

// ---- vibe / LED ----
uint32_t vibeOffAt   = 0;
uint32_t ledPulseEnd = 0;

bool fatalError = false;
uint32_t fatalBlinkNext = 0;
bool fatalBlinkState = false;

// =====================================================
static inline void vibeKick() {
    digitalWrite(VIBE_PIN, HIGH);
    vibeOffAt = millis() + VIBE_MS;
}
static inline void vibeService() {
    if (vibeOffAt && (int32_t)(millis() - vibeOffAt) >= 0) {
        digitalWrite(VIBE_PIN, LOW);
        vibeOffAt = 0;
    }
}
static inline void ledPulseKick(uint16_t ms) {
    digitalWrite(LED_PIN, LED_ON);
    ledPulseEnd = millis() + ms;
}

void led_update() {
    uint32_t now = millis();

    if (fatalError) {
        if ((int32_t)(now - fatalBlinkNext) >= 0) {
            fatalBlinkState = !fatalBlinkState;
            digitalWrite(LED_PIN, fatalBlinkState ? LED_ON : LED_OFF);
            fatalBlinkNext = now + 80;
        }
        return;
    }

    if (ledPulseEnd) {
        if ((int32_t)(now - ledPulseEnd) < 0) {
            digitalWrite(LED_PIN, LED_ON);
            return;
        }
        ledPulseEnd = 0;
    }

    switch (currentMode) {
        case MODE_BOOT: {
            if (bootPhase == 0) {
                digitalWrite(LED_PIN, ((now / 400) & 1) ? LED_ON : LED_OFF);
            } else if (bootPhase == 1) {
                digitalWrite(LED_PIN, ((now / 150) & 1) ? LED_ON : LED_OFF);
            } else {
                digitalWrite(LED_PIN, LED_OFF);
            }
            break;
        }
        case MODE_WIFI_OPTIONS:
        case MODE_NETWORK:
        case MODE_SCAN_QR:
        case MODE_QR_CONFIRM:
            digitalWrite(LED_PIN, LED_ON);
            break;
        case MODE_ALERT: {
            uint32_t p = now % 1000;
            bool on = (p < 60) || (p >= 120 && p < 180) || (p >= 240 && p < 300);
            digitalWrite(LED_PIN, on ? LED_ON : LED_OFF);
            break;
        }
        case MODE_SD_MISSING: {
            uint32_t p = now % 500;
            digitalWrite(LED_PIN, (p < 80) ? LED_ON : LED_OFF);
            break;
        }
        default:
            digitalWrite(LED_PIN, LED_OFF);
            break;
    }
}

static void led_powerOnBlip() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, LED_ON);
        delay(80);
        digitalWrite(LED_PIN, LED_OFF);
        delay(80);
    }
}

// =====================================================
void updateWifiState() {
    wifi_mode_t m = WiFi.getMode();

    bool nowOnline = (WiFi.softAPgetStationNum() > 0);
    bool nowAp     = (m == WIFI_AP) || (m == WIFI_AP_STA);

    if (nowOnline != g_wifiOnline || nowAp != g_wifiApActive) {
        g_wifiOnline   = nowOnline;
        g_wifiApActive = nowAp;
        if (currentMode == MODE_MENU) {
            menu_statusPatch(g_wifiOnline, g_wifiApActive);
        }
    }
}

// =====================================================
// GPIO 3
// =====================================================
bool updateBtn3() {
    bool reading = digitalRead(BTN_SCREEN);
    uint32_t now = millis();

    if (reading == LOW && btn3LastReading == HIGH) {
        btn3PressStart = now;
        btn3Held = true;
        btn3LongPressTriggered = false;
    }
    else if (reading == HIGH && btn3LastReading == LOW) {
        bool wasLongPress = btn3LongPressTriggered;
        bool wasShort = (now - btn3PressStart) < LONG_PRESS_MS;
        btn3Held = false;
        btn3LongPressTriggered = false;
        btn3LastReading = reading;
        if (!wasLongPress && wasShort) return true;
        return false;
    }

    btn3LastReading = reading;
    return false;
}

// =====================================================
// GPIO 5
// =====================================================
bool checkBtn5LongPress() {
    if (btn5Held && !btn5LongPressTriggered) {
        if (millis() - btn5PressStart >= LONG_PRESS_MS) {
            btn5LongPressTriggered = true;
            vibeKick();
            return true;
        }
    }
    return false;
}

bool updateBtn5() {
    bool reading = digitalRead(BTN_SELECT);
    uint32_t now = millis();

    if (reading == LOW && btn5LastReading == HIGH) {
        btn5PressStart = now;
        btn5Held = true;
        btn5LongPressTriggered = false;
    }
    else if (reading == HIGH && btn5LastReading == LOW) {
        bool wasLongPress = btn5LongPressTriggered;
        bool wasShort = (now - btn5PressStart) < LONG_PRESS_MS;
        btn5Held = false;
        btn5LongPressTriggered = false;
        btn5LastReading = reading;
        if (!wasLongPress && wasShort) return true;
        return false;
    }

    btn5LastReading = reading;
    return false;
}

// =====================================================
bool jpg_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= FRAME_H || x >= FRAME_W) return 1;
    if (x + w > FRAME_W) w = FRAME_W - x;
    if (y + h > FRAME_H) h = FRAME_H - y;
    for (int row = 0; row < h; row++)
        memcpy(streamBuf + (y + row) * FRAME_W + x,
               bitmap + row * w, w * 2);
    return 1;
}

void blitFrame() {
    for (int dstY = 0; dstY < ROW_STRETCH; dstY++) {
        int srcY = (dstY * FRAME_H) / ROW_STRETCH;
        if (srcY >= FRAME_H) srcY = FRAME_H - 1;
        memcpy(tftBuf + dstY * FRAME_W,
               streamBuf + srcY * FRAME_W,
               FRAME_W * 2);
    }
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, FRAME_W, ROW_STRETCH, tftBuf);
}

void showAlert(const char* msg, Mode returnTo, int returnTile) {
    Serial.printf("[ALERT] %s\n", msg);
    menu_drawAlert(msg);
    currentMode = MODE_ALERT;
    alertShowUntil = millis() + 2000;
    alertReturnMode = returnTo;
    alertReturnTile = returnTile;
}

// =====================================================
// AP visibility
// =====================================================
void applyApVisibility(bool visible) {
    WiFi.softAP(AP_SSID, AP_PASS, ESPNOW_CHANNEL, /*hidden=*/!visible);
    Serial.printf("[AP] %s  ssid=%s  ch=%d  ip=%s\n",
                  visible ? "VISIBLE" : "HIDDEN",
                  AP_SSID,
                  ESPNOW_CHANNEL,
                  WiFi.softAPIP().toString().c_str());
}

// =====================================================
// NETWORK actions
// =====================================================
void network_applyWifiEnable(bool on) {
    g_wifiEnabled = on;
    applyApVisibility(on);
    Serial.printf("[NET] WIFI = %s\n", on ? "ON" : "OFF");
    updateWifiState();
}

void network_actionTest() {
    int clients = WiFi.softAPgetStationNum();
    Serial.printf("[NET] TEST -> AP stations = %d\n", clients);
}

// --- Start scan QR flow ---
void network_startScanQr() {
    Serial.println("[QR] starting scan");
    qrSsid = "";
    qrPass = "";

    // Ask CAM to switch to grayscale + start QR decode
    espnow_sendCommand("SCAN_QR_START");

    tft.startWrite();
    tft.fillScreen(0x0000);
    menu_drawQrFrame();
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setFreeFont(&Petme8x8);
    tft.setTextColor(0xF206, 0x0000);
    tft.drawCentreString("SCAN QR", 80, 8, 1);
    tft.endWrite();

    currentMode = MODE_SCAN_QR;
    qrScanStartMs = millis();
    framesThisSec = 0;
}

// --- Stop scan and go back to Network ---
void network_stopScanQr(bool toConfirm) {
    espnow_sendCommand("SCAN_QR_STOP");

    if (!toConfirm) {
        currentMode = MODE_NETWORK;
        menu_drawNetwork(g_netSelected);
    }
}

// --- Handle received QR credentials ---
void network_handleQrData(const String& ssid, const String& pass) {
    Serial.printf("[QR] received: ssid='%s' pass='%s'\n",
                  ssid.c_str(), pass.c_str());

    qrSsid = ssid;
    qrPass = pass;
    qrYesSelected = true;

    espnow_sendCommand("SCAN_QR_STOP");
    currentMode = MODE_QR_CONFIRM;
    menu_drawQrConfirm(qrSsid.c_str(), qrPass.c_str(), qrYesSelected);
}

// --- Confirm handlers ---
void network_confirmYes() {
    Serial.printf("[QR] SAVED  ssid='%s'  pass='%s'\n",
                  qrSsid.c_str(), qrPass.c_str());
    // TODO: real WiFi.begin() lands with the CAM firmware.
    menu_showNotification("SAVED", qrSsid.c_str(), 0x8E09, 1500);
    currentMode = MODE_NOTIFY;
    notifyUntil = millis() + 1500;
    notifyReturnMode = MODE_NETWORK;
    notifyReturnTile = -1;
}

void network_confirmNo() {
    Serial.println("[QR] rejected — resuming scan");
    network_startScanQr();
}

// =====================================================
// Serial test stub:  "QR <ssid> <pass>"
// =====================================================
void pollSerialQrStub() {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (!line.startsWith("QR ")) return;

    int firstSpace = line.indexOf(' ');
    int secondSpace = line.indexOf(' ', firstSpace + 1);

    String ssid, pass;
    if (secondSpace < 0) {
        ssid = line.substring(firstSpace + 1);
        pass = "";
    } else {
        ssid = line.substring(firstSpace + 1, secondSpace);
        pass = line.substring(secondSpace + 1);
    }

    Serial.printf("[QR-STUB] simulating scan: ssid='%s' pass='%s'\n",
                  ssid.c_str(), pass.c_str());
    network_handleQrData(ssid, pass);
}

// =====================================================
// GPIO 3 SHORT PRESS
// =====================================================
void handleBtn3ShortPress() {
    vibeKick();
    Serial.println("[BTN3] cycle");

    if (currentMode == MODE_MENU) {
        int old = menuTile;
        menuTile = (menuTile + 1) % 6;
        menu_redrawPatch(old, menuTile);
    }
    else if (currentMode == MODE_WIFI_OPTIONS) {
        int old = wifiOptSel;
        wifiOptSel = (wifiOptSel + 1) % 2;
        menu_wifiOptionsRedraw(old, wifiOptSel);
        ledPulseKick(40);
    }
    else if (currentMode == MODE_NETWORK) {
        int old = g_netSelected;
        g_netSelected = (g_netSelected + 1) % NET_ROW_COUNT;
        menu_networkPatch(old, g_netSelected);
        ledPulseKick(40);
    }
    else if (currentMode == MODE_QR_CONFIRM) {
        qrYesSelected = !qrYesSelected;
        menu_qrConfirmPatch(qrYesSelected);
        ledPulseKick(40);
    }
    else if (currentMode == MODE_SCAN_QR) {
        // cancel scan
        network_stopScanQr(false);
    }
    else if (currentMode == MODE_POPUP) {
        popupYes = !popupYes;
        menu_drawPopupPatch(popupYes);
    }
    else if (currentMode == MODE_PREVIEW) {
        espnow_sendCommand("STOP_STREAM");
        currentMode = MODE_MENU;
        menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
        clock_startTick();
    }
    else if (currentMode == MODE_DIAG) {
        currentMode = MODE_MENU;
        menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
        clock_startTick();
    }
    else if (currentMode == MODE_SD_MISSING) {
        currentMode = MODE_MENU;
        menuTile = 2;
        menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
        clock_startTick();
    }
    else if (currentMode == MODE_ALERT) {
        currentMode = alertReturnMode;
        if (currentMode == MODE_MENU) {
            menuTile = alertReturnTile;
            menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
            clock_startTick();
        }
    }
}

// =====================================================
// GPIO 5 SHORT PRESS
// =====================================================
void handleBtn5Select() {
    vibeKick();
    Serial.println("[BTN5] select");

    if (currentMode == MODE_MENU) {
        if (menuTile == 0) {
            loadPurpose = LOAD_FOR_PREVIEW;
            bool sent = espnow_sendCommand("START_STREAM");
            if (!sent) { showAlert("Error 101", MODE_MENU, 0); return; }
            menu_drawLoader();
            currentMode = MODE_LOADING;
            lastLoaderFrame = -1;
            lastFrameTime = millis();
            framesThisSec = 0;
        }
        else if (menuTile == 1) {
            wifiOptSel = 0;
            currentMode = MODE_WIFI_OPTIONS;
            menu_drawWifiOptions(wifiOptSel);
        }
        else if (menuTile == 2) {
            loadPurpose = LOAD_FOR_LOCAL_SD_MISSING;
            menu_drawLoader();
            currentMode = MODE_LOADING;
            lastLoaderFrame = -1;
            lastFrameTime = millis();
        }
        else if (menuTile == 4) {
            currentMode = MODE_DIAG;
            menu_drawDiag(espnow_isConnected(), sdPresent);
        }
        else {
            menu_showNotification("SOON", "not impl", 0xFFE0, 2000,
                                 MODE_MENU, menuTile);
            currentMode = MODE_NOTIFY;
            notifyUntil = millis() + 2000;
            notifyReturnMode = MODE_MENU;
            notifyReturnTile = menuTile;
        }
    }
    else if (currentMode == MODE_WIFI_OPTIONS) {
        if (wifiOptSel == 0) {
            g_netSelected = 0;
            currentMode = MODE_NETWORK;
            menu_drawNetwork(g_netSelected);
        } else {
            menu_showNotification("SOON", "SCAN", 0xFFE0, 1500);
            currentMode = MODE_NOTIFY;
            notifyUntil = millis() + 1500;
            notifyReturnMode = MODE_WIFI_OPTIONS;
            notifyReturnTile = -1;
            ledPulseKick(60);
        }
    }
    else if (currentMode == MODE_NETWORK) {
        switch (g_netSelected) {
            case 0:
                network_applyWifiEnable(!g_wifiEnabled);
                menu_networkRefreshRow(0);
                ledPulseKick(60);
                break;
            case 1:
                network_startScanQr();
                ledPulseKick(60);
                break;
            case 2:
                // Mode row is read-only
                ledPulseKick(40);
                break;
            case 3:
                network_actionTest();
                ledPulseKick(60);
                break;
        }
    }
    else if (currentMode == MODE_QR_CONFIRM) {
        if (qrYesSelected) network_confirmYes();
        else               network_confirmNo();
        ledPulseKick(60);
    }
    else if (currentMode == MODE_POPUP) {
        if (popupYes) {
            currentMode = MODE_PREVIEW;
            tft.fillScreen(0x0000);
            lastFrameTime = millis();
        } else {
            espnow_sendCommand("STOP_STREAM");
            currentMode = MODE_MENU;
            menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
            clock_startTick();
        }
    }
    else if (currentMode == MODE_DIAG) {
        menu_drawDiag(espnow_isConnected(), sdPresent);
    }
    else if (currentMode == MODE_SD_MISSING) {
        currentMode = MODE_MENU;
        menuTile = 2;
        menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
        clock_startTick();
    }
    else if (currentMode == MODE_ALERT) {
        currentMode = alertReturnMode;
        if (currentMode == MODE_MENU) {
            menuTile = alertReturnTile;
            menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
            clock_startTick();
        }
    }
}

// =====================================================
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n==== DuckyMini (ESP-NOW) ====");

    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, LOW);

    pinMode(VIBE_PIN, OUTPUT);
    digitalWrite(VIBE_PIN, LOW);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_OFF);

    led_powerOnBlip();

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.invertDisplay(false);
    tft.fillScreen(0x0000);

    pinMode(BTN_SCREEN, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    streamBuf = (uint16_t*)malloc(FRAME_W * FRAME_H * 2);
    tftBuf    = (uint16_t*)malloc(FRAME_W * ROW_STRETCH * 2);
    if (!streamBuf || !tftBuf) {
        Serial.println("[!] OOM buffers");
        fatalError = true;
        while (true) { led_update(); delay(5); }
    }

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(jpg_output);

    digitalWrite(BACKLIGHT_PIN, HIGH);
    bootStart = millis();
    bootPhase = 0;
    menu_drawLogoScreen();

    if (!espnow_init()) {
        Serial.println("[!] ESP-NOW init failed");
        fatalError = true;
        while (true) { led_update(); delay(5); }
    }

    WiFi.mode(WIFI_AP_STA);
    applyApVisibility(false);
    g_wifiEnabled = false;
    WiFi.setTxPower(WIFI_POWER_11dBm);

    clock_init();
    updateWifiState();

    Serial.println("[SYS] boot sequence started");
    Serial.println("[SYS] QR test: type  QR <ssid> <pass>  to simulate");
}

// =====================================================
void loop() {
    vibeService();
    led_update();

    if (currentMode == MODE_BOOT) {
        uint32_t elapsed = millis() - bootStart;

        if (bootPhase == 0 && elapsed >= 2000) {
            bootPhase = 1;
            menu_drawLoader();
            lastLoaderFrame = -1;
        }
        else if (bootPhase == 1 && elapsed >= 4500) {
            bootPhase = 2;
            menu_drawBlackScreen();
        }
        else if (bootPhase == 2 && elapsed >= 5500) {
            bootPhase = 3;
            currentMode = MODE_MENU;
            updateWifiState();
            menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
            clock_startTick();
        }

        if (bootPhase == 1) menu_pumpLoader(&lastLoaderFrame);

        delay(5);
        return;
    }

    pollSerialQrStub();

    bool shortPressed3 = updateBtn3();
    if (shortPressed3) handleBtn3ShortPress();

    bool longPressed5  = checkBtn5LongPress();
    bool shortPressed5 = updateBtn5();

    if (longPressed5) {
        if (currentMode == MODE_QR_CONFIRM) {
            Serial.println("[QR] back to scan");
            network_startScanQr();
        }
        else if (currentMode == MODE_SCAN_QR) {
            Serial.println("[QR] cancel scan");
            network_stopScanQr(false);
        }
        else if (currentMode == MODE_NETWORK) {
            currentMode = MODE_WIFI_OPTIONS;
            menu_drawWifiOptions(wifiOptSel);
        }
        else if (currentMode == MODE_WIFI_OPTIONS) {
            currentMode = MODE_MENU;
            menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
            clock_startTick();
        }
    }
    else if (shortPressed5) {
        handleBtn5Select();
    }

    if (currentMode == MODE_LOADING) {
        menu_pumpLoader(&lastLoaderFrame);

        if (loadPurpose == LOAD_FOR_PREVIEW) {
            if (millis() - lastFrameTime > 200) {
                currentMode = MODE_POPUP;
                popupYes = false;
                menu_drawPopupFull(popupYes);
            }
        }
        else if (loadPurpose == LOAD_FOR_LOCAL_SD_MISSING) {
            if (millis() - lastFrameTime > 500) {
                menu_drawSdMissing();
                currentMode = MODE_SD_MISSING;
            }
        }
    }

    if (currentMode == MODE_MENU) {
        clock_tick(&tft, &clockSprite);
        updateWifiState();
    }

    // Scan QR timeout — 30 s from when the scan started
    if (currentMode == MODE_SCAN_QR &&
        millis() - qrScanStartMs > 30000) {
        Serial.println("[QR] timeout");
        network_stopScanQr(false);
    }

    if (currentMode == MODE_NOTIFY && millis() > notifyUntil) {
        currentMode = notifyReturnMode;
        if (currentMode == MODE_MENU) {
            menuTile = notifyReturnTile;
            menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
            clock_startTick();
        }
        else if (currentMode == MODE_WIFI_OPTIONS) {
            menu_drawWifiOptions(wifiOptSel);
        }
        else if (currentMode == MODE_NETWORK) {
            menu_drawNetwork(g_netSelected);
        }
    }

    if (currentMode == MODE_ALERT && millis() > alertShowUntil) {
        currentMode = alertReturnMode;
        if (currentMode == MODE_MENU) {
            menuTile = alertReturnTile;
            menu_drawFull(menuTile, g_wifiOnline, g_wifiApActive);
            clock_startTick();
        }
    }

    if (currentMode == MODE_PREVIEW && millis() - lastFrameTime > 3000) {
        espnow_sendCommand("STOP_STREAM");
        showAlert("Error 102", MODE_MENU, menuTile);
    }

    if (currentMode == MODE_DIAG && millis() - diagRefresh > 2000) {
        diagRefresh = millis();
        menu_drawDiag(espnow_isConnected(), sdPresent);
    }

    EspNowMsg m;
    if (espnow_poll(&m)) {
        if (m.isText) {
            String cmd((char*)m.data, m.len);
            cmd.trim();
            Serial.printf("[MINI<-CAM] %s\n", cmd.c_str());

            if (cmd.startsWith("QR_DATA ")) {
                String rest = cmd.substring(8);
                int sp = rest.indexOf(' ');
                String ssid, pass;
                if (sp < 0) { ssid = rest; pass = ""; }
                else        { ssid = rest.substring(0, sp);
                              pass = rest.substring(sp + 1); }
                network_handleQrData(ssid, pass);
            }
            else if (cmd == "PHOTO SAVED")
                menu_showNotification("SAVED!", "check gallery", 0x8E09, 1500, MODE_PREVIEW);
            else if (cmd == "STORAGE FULL")
                menu_showNotification("STORAGE", "FULL", 0xF206, 2500, MODE_PREVIEW);
            else if (cmd == "CAPTURE FAILED")
                showAlert("Error 103", MODE_MENU, menuTile);
            else if (cmd == "SD_FOUND") {
                sdPresent = true;
                currentMode = MODE_NOTIFY;
                notifyUntil = millis() + 1500;
                notifyReturnMode = MODE_MENU;
                notifyReturnTile = 2;
                menu_showNotification("SD OK", "card found", 0x8E09, 1500, MODE_MENU, 2);
            }
            else if (cmd == "SD_MISSING") {
                sdPresent = false;
                menu_drawSdMissing();
                currentMode = MODE_SD_MISSING;
            }
            free(m.data);
        } else {
            if ((currentMode == MODE_PREVIEW || currentMode == MODE_SCAN_QR) &&
                m.len >= 4 &&
                m.data[0] == 0xFF && m.data[1] == 0xD8) {
                lastFrameTime = millis();
                TJpgDec.drawJpg(0, 0, m.data, m.len);
                blitFrame();

                if (currentMode == MODE_SCAN_QR) {
                    tft.startWrite();
                    menu_drawQrFrame();
                    tft.setTextColor(0xF206, 0x0000);
                    tft.setTextSize(1);
                    tft.setFreeFont(&Petme8x8);
                    tft.drawCentreString("SCAN QR", 80, 8, 1);
                    tft.endWrite();
                }

                framesThisSec++;
                uint32_t now = millis();
                if (now - lastFpsCalc >= 1000) {
                    fps = framesThisSec;
                    framesThisSec = 0;
                    lastFpsCalc = now;
                }
            }
            free(m.data);
        }
    }

    delay(5);
}