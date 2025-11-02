#ifndef _Display_text_H_
#define _Display_text_H_

#pragma once
#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define RGB_Control_PIN   14  

// Display_text.cppで定義されるMatrixオブジェクト
extern Adafruit_NeoMatrix Matrix;

// 文字幅計算ユーティリティ
int getCharWidth(char c);
int getStringWidth(const char* str);

// テキスト表示関連
void Text_Flow(char* Text);
void Matrix_Init();   
void Text_PlayOnce(const char* text, uint16_t frame_delay_ms);

// 起動確認の全点灯テスト（指定RGBでduration_msミリ秒点灯→消灯）
void Matrix_BootTest(uint8_t r, uint8_t g, uint8_t b, uint16_t duration_ms);

// 表示色を colors[] のインデックスで切り替え（0:白,1:緑,2:青）
void Matrix_SetTextColorIndex(uint8_t idx);

// === テキスト用の明るさ ===
extern uint8_t gTextBrightness;      // テキスト/スクロール用

void Matrix_SetTextBrightness(uint8_t b);

#endif
