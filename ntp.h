#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

static uint32_t clockLastTick = 0;
static bool     clockInited   = false;

void clock_init() {
    clockLastTick = 0;
    clockInited = true;
}

void clock_startTick() {
    clockLastTick = millis();
}

void clock_redraw(TFT_eSPI* t, TFT_eSprite* spr) {
    if (!clockInited) return;

    static bool spriteReady = false;
    if (!spriteReady) {
        spr->createSprite(160, 43);
        spriteReady = true;
    }

    spr->fillSprite(0x0000);
    spr->setTextColor(0xF206);
    spr->setFreeFont(NULL);

    spr->setTextSize(2);
    spr->drawCentreString("--:--", 80, 6, 1);

    spr->setTextSize(1);
    spr->drawCentreString("offline", 80, 23, 1);

    spr->pushSprite(0, 10);
}

void clock_tick(TFT_eSPI* t, TFT_eSprite* spr) {
    if (!clockInited) return;
    uint32_t now = millis();
    if (now - clockLastTick > 1000) {
        clockLastTick = now;
        clock_redraw(t, spr);
    }
}