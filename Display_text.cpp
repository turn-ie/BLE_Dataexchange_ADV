#include "Display_text.h"
#include <math.h>

// === テキスト用の明るさ ===
uint8_t gTextBrightness = 1;  // テキスト/スクロール

void Matrix_SetTextBrightness(uint8_t b){ gTextBrightness = b; }

Adafruit_NeoMatrix Matrix = Adafruit_NeoMatrix(8, 8, RGB_Control_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800);

int MatrixWidth = 0;
const uint16_t colors[] = {
  Matrix.Color(255,255,255), Matrix.Color(0,255,0), Matrix.Color(0,0,255) };

int getCharWidth(char c){ return (c=='i'||c=='l'||c=='!'||c=='.')?3:5; }
int getStringWidth(const char* s){ int w=0; for(int i=0,l=strlen(s); i<l; ++i){ w+=getCharWidth(s[i])+1; } return w; }

void Matrix_Init(){
  Matrix.begin();
  Matrix.setTextWrap(false);
  Matrix.setBrightness(gTextBrightness);
  Matrix.setTextColor(colors[0]); // テキストは白固定
  MatrixWidth = Matrix.width();
}

void Text_Flow(char* Text){
  Matrix.setBrightness(gTextBrightness);
  int textWidth = getStringWidth(Text);
  Matrix.fillScreen(0);
  Matrix.setCursor(MatrixWidth,0);
  Matrix.print(Text);
  if(--MatrixWidth < -textWidth) MatrixWidth = Matrix.width();
  Matrix.show();
}

void Text_PlayOnce(const char* text,uint16_t frame_delay_ms){
  Matrix.setBrightness(gTextBrightness);
  int textWidth=getStringWidth(text); 
  MatrixWidth=Matrix.width();
  while(MatrixWidth >= -textWidth){ 
    Matrix.fillScreen(0); 
    Matrix.setCursor(MatrixWidth,0); 
    Matrix.print(text); 
    Matrix.show(); 
    MatrixWidth--; 
    delay(frame_delay_ms);
  } 
}

