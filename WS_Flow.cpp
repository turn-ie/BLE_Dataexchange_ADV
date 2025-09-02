#include "WS_Flow.h"
#include <math.h> // 追加: リップル計算用


Adafruit_NeoMatrix Matrix = Adafruit_NeoMatrix(8, 8, RGB_Control_PIN, 
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +                     
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,                
  NEO_GRB            + NEO_KHZ800);                           

int MatrixWidth = 0;
const uint16_t colors[] = {
  Matrix.Color(255, 0, 0), Matrix.Color(0, 255, 0), Matrix.Color(0, 0, 255) };

int getCharWidth(char c) {
  if (c == 'i' || c == 'l' || c == '!' || c == '.') {
    return 3;
  } else {
    return 5;
  }
}
int getStringWidth(const char* str) {
  int width = 0;
  int length = strlen(str);
  for (int i = 0; i < length; i++) {
    width += getCharWidth(str[i]);
    width += 1;      
  }
  return width;
}
void Matrix_Init() {
  Matrix.begin();
  Matrix.setTextWrap(false);  
  Matrix.setBrightness(1);           // 通常時の明るさ
  Matrix.setTextColor(colors[0]); 
  MatrixWidth   = Matrix.width(); 
}
void Text_Flow(char* Text) {
  int textWidth   = getStringWidth(Text);
  Matrix.fillScreen(0);
  Matrix.setCursor(MatrixWidth, 0);
  Matrix.print(Text); // 修正: F(Text)はリテラル専用
  if (--MatrixWidth < -textWidth) {
    MatrixWidth = Matrix.width();
  }
  Matrix.show();
}

// --- ユーティリティ: ガンマ補正 ---
static inline uint8_t gamma8(float v01){
  if (v01 < 0.f) v01 = 0.f;
  if (v01 > 1.f) v01 = 1.f;
  float g = powf(v01, 1.0f/2.2f);
  return (uint8_t)(g * 255.0f + 0.5f);
}

// --- ユーティリティ: HSV(0–255)→RGB ---
static uint16_t ColorHSV8(uint8_t h, uint8_t s, uint8_t v){
  uint8_t region = h / 43;                 // 0..5
  uint8_t rem    = (h - region * 43) * 6;  // 0..255
  uint8_t p = (uint16_t)v * (255 - s) / 255;
  uint8_t q = (uint16_t)v * (255 - ((uint16_t)s * rem) / 255) / 255;
  uint8_t t = (uint16_t)v * (255 - ((uint16_t)s * (255 - rem)) / 255) / 255;

  uint8_t r, g, b;
  switch(region){
    default:
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
  }
  return Matrix.Color(r, g, b);
}

// --- リップルを1サイクルだけ再生 → フェードアウトして消灯 ---
void Ripple_PlayOnce(uint16_t /*duration_ms_ignored*/){
  // パラメータ（提示コード準拠）
  const uint8_t GLOBAL_BRIGHTNESS = 22;   // 再生中の明るさ
  const uint8_t LEVELS = 12;              // 明度段階
  const float SPEED   = 0.14f;            // 拡がる速さ
  const float SPACING = 0.85f;            // 輪の間隔
  const float SIGMA   = 0.55f;            // 輪の太さ
  const int   RINGS   = 4;                // 重ねる輪の本数
  const float MAX_DIST = 4.95f;
  const float PERIOD   = MAX_DIST + (RINGS-1)*SPACING + 2.0f*SIGMA; // 1サイクル尺

  // 色相（任意開始、次回に向けて進める）
  static uint8_t currentHue = 240;  // おまかせ
  const uint8_t HUE_STEP = 24;

  // 現在の明るさを保存し、再生中だけ上げる
  uint8_t prevBrightness = 1; // デフォルト初期値
  // Adafruit_NeoMatrixに現在値を取得するAPIは無いので、設計上の通常値を使用
  Matrix.setBrightness(GLOBAL_BRIGHTNESS);

  // 中心座標
  const float cx = (Matrix.width()  - 1) * 0.5f; // 8x8なら3.5
  const float cy = (Matrix.height() - 1) * 0.5f;

  float t = 0.f; // 時間位相

  // 1サイクル分レンダリング
  while (t <= PERIOD) {
    for (int y = 0; y < Matrix.height(); y++) {
      for (int x = 0; x < Matrix.width(); x++) {
        float dx = (float)x - cx;
        float dy = (float)y - cy;
        float dist = sqrtf(dx*dx + dy*dy);

        float amp = 0.f;
        for (int k = 0; k < RINGS; k++) {
          float r = t - k*SPACING;
          float d = dist - r;
          amp += expf(-(d*d) / (2.f*SIGMA*SIGMA));
        }
        if (amp > 1.f) amp = 1.f;

        // 多階調＋ガンマ
        float stepped = floorf(amp * LEVELS) / LEVELS;

        // 距離で彩度を少し落とす
        float satf = 0.90f - 0.25f * (dist / 4.8f);
        if (satf < 0.f) satf = 0.f;
        if (satf > 1.f) satf = 1.f;

        uint8_t V = gamma8(stepped * 0.9f);
        // ほんの少しだけ柔らかく（nscale8_video(250)相当）
        V = (uint8_t)((V * 250 + 127) / 255);
        uint8_t S = (uint8_t)(satf * 255.f + 0.5f);

        uint16_t c = ColorHSV8(currentHue, S, V);
        Matrix.drawPixel(x, y, c);
      }
    }
    Matrix.show();
    delay(20);

    // 位相更新
    t += SPEED;
  }

  // サイクル完了時に色相を進める
  currentHue += HUE_STEP; // 0–255 で循環は自動

  // フェードアウト（明るさを徐々に下げて完全消灯）
  for (int b = GLOBAL_BRIGHTNESS; b >= 0; b -= 2) {
    Matrix.setBrightness(b);
    Matrix.show();
    delay(18);
  }
  Matrix.fillScreen(0);
  Matrix.show();

  // 明るさを元に戻す
  Matrix.setBrightness(prevBrightness);
}

// --- 追加: テキストを最後までスクロール再生（ブロッキング） ---
void Text_PlayOnce(const char* text, uint16_t frame_delay_ms) {
  int textWidth = getStringWidth(text);
  MatrixWidth   = Matrix.width();

  while (MatrixWidth >= -textWidth) {
    Matrix.fillScreen(0);
    Matrix.setCursor(MatrixWidth, 0);
    Matrix.print(text);
    Matrix.show();
    MatrixWidth--;
    delay(frame_delay_ms);
  }
}
