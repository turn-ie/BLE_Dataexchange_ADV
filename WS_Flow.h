#ifndef _WS_Flow_H_
#define _WS_Flow_H_

#pragma once
#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define RGB_Control_PIN   14  

int getCharWidth(char c);
int getStringWidth(const char* str);
void Text_Flow(char* Text);
void Matrix_Init();   
void Ripple_PlayOnce(uint16_t duration_ms);
void Radar_PlayOnce();
// 追加: テキストを最後までスクロール再生（ブロッキング）
void Text_PlayOnce(const char* text, uint16_t frame_delay_ms);

// 起動確認の全点灯テスト（指定RGBでduration_msミリ秒点灯→消灯）
void Matrix_BootTest(uint8_t r, uint8_t g, uint8_t b, uint16_t duration_ms);

// 追加: 表示色を colors[] のインデックスで切り替え（0:赤,1:緑,2:青）
void Matrix_SetTextColorIndex(uint8_t idx);

// === 新: 明るさ2系統 + モーション色相共有 ===
extern uint8_t gTextBrightness;      // テキスト/スクロール用
extern uint8_t gMotionBrightness;    // レーダー/リップル共通
extern uint8_t gMotionHue;           // レーダー/リップル共有色相

void Matrix_SetTextBrightness(uint8_t b);
void Matrix_SetMotionBrightness(uint8_t b);
void Motion_SetHue(uint8_t h);

// レーダー待機(非ブロッキング)
void Radar_InitIdle();
void Radar_IdleStep(bool doShow = true);

#endif
