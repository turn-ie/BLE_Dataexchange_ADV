#include "WS_Flow.h"
#include <math.h>

// === 新: 2系統明るさ + 共通色相 ===
uint8_t gTextBrightness   = 30;  // テキスト/スクロール
uint8_t gMotionBrightness = 22;  // レーダー/リップル
uint8_t gMotionHue        = 90; // 共通色相 (固定用)
// Hue 0-255 早見表 (HSV H→代表色 / 角度近似)
//100ピンク
//90赤ピンク
//210水色
//235青緑

void Matrix_SetTextBrightness(uint8_t b){ gTextBrightness = b; }
void Matrix_SetMotionBrightness(uint8_t b){ gMotionBrightness = b; }
void Motion_SetHue(uint8_t h){ gMotionHue = h; }

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

static inline uint8_t gamma8(float v01){ if(v01<0) v01=0; if(v01>1) v01=1; float g=powf(v01,1.0f/2.2f); return (uint8_t)(g*255.f+0.5f); }
static uint16_t ColorHSV8(uint8_t h,uint8_t s,uint8_t v){
  uint8_t region=h/43; uint8_t rem=(h-region*43)*6; uint8_t p=(uint16_t)v*(255-s)/255;
  uint8_t q=(uint16_t)v*(255-((uint16_t)s*rem)/255)/255; uint8_t t=(uint16_t)v*(255-((uint16_t)s*(255-rem))/255)/255;
  uint8_t r,g,b; switch(region){ default: case 0:r=v; g=t; b=p; break; case 1:r=q; g=v; b=p; break; case 2:r=p; g=v; b=t; break; case 3:r=p; g=q; b=v; break; case 4:r=t; g=p; b=v; break; case 5:r=v; g=p; b=q; break; }
  return Matrix.Color(r,g,b);
}

// （任意）旧ブロッキングRadar（色固定化のため色相更新は行わない）
void Radar_PlayOnce(){
  const float SPEED=2.5f; const float BW_F=0.8f; const float BW_B=0.05f; const uint8_t FADE=10; const uint16_t DT=16; // const uint8_t HSTEP=24; // 未使用
  Matrix.setBrightness(gMotionBrightness);
  float ang=0; uint8_t hue=gMotionHue; const float cx=(Matrix.width()-1)*0.5f; const float cy=(Matrix.height()-1)*0.5f;
  while(ang<360.f){
    for(int i=0;i<Matrix.width()*Matrix.height();++i){ uint32_t col=Matrix.getPixelColor(i); if(col){ uint8_t r=(col>>16)&0xFF,g=(col>>8)&0xFF,b=col&0xFF; r=(r<=FADE)?0:r-FADE; g=(g<=FADE)?0:g-FADE; b=(b<=FADE)?0:b-FADE; Matrix.setPixelColor(i,Matrix.Color(r,g,b)); }}
    float rad=ang*(float)M_PI/180.f;
    for(uint8_t y=0;y<Matrix.height();++y) for(uint8_t x=0;x<Matrix.width();++x){ float dx=x-cx,dy=y-cy; float pr=atan2f(dy,dx); float diff=rad-pr; while(diff>M_PI) diff-=2.f*M_PI; while(diff<-M_PI) diff+=2.f*M_PI; float bw=(diff>0)?BW_F:BW_B; float br=expf(-(diff*diff)/(2.f*bw*bw)); if(br>0.05f){ uint8_t V=gamma8(br); uint16_t c=ColorHSV8(hue,255,V); Matrix.drawPixel(x,y,c);} }
    Matrix.show(); ang+=SPEED; if(ang>=360.f){ break;} delay(DT);
  }
  // 色相を進めず固定
  Matrix.fillScreen(0); Matrix.show();
  Matrix.setBrightness(gTextBrightness);
}

void Ripple_PlayOnce(uint16_t){
  const uint8_t LEVELS=12; const float SPEED=0.14f; const float SPACING=0.85f; const float SIGMA=0.55f; const int RINGS=4; const float MAX_DIST=4.95f;
  const float PERIOD=MAX_DIST+(RINGS-1)*SPACING+2.f*SIGMA; // const uint8_t HSTEP=24; // 色固定なので未使用
  Matrix.setBrightness(gMotionBrightness);
  uint8_t localHue = gMotionHue; // 固定色相
  const float cx=(Matrix.width()-1)*0.5f; const float cy=(Matrix.height()-1)*0.5f; float t=0.f;
  while(t<=PERIOD){
    for(int y=0;y<Matrix.height();++y){ for(int x=0;x<Matrix.width();++x){ float dx=x-cx, dy=y-cy; float dist=sqrtf(dx*dx+dy*dy); float amp=0.f; for(int k=0;k<RINGS;k++){ float r=t-k*SPACING; float d=dist-r; amp+=expf(-(d*d)/(2.f*SIGMA*SIGMA)); } if(amp>1.f) amp=1.f; float stepped=floorf(amp*LEVELS)/LEVELS; float satf=0.90f-0.25f*(dist/4.8f); if(satf<0) satf=0; if(satf>1) satf=1; uint8_t V=gamma8(stepped*0.9f); V=(uint8_t)((V*250+127)/255); uint8_t S=(uint8_t)(satf*255.f+0.5f); uint16_t c=ColorHSV8(localHue,S,V); Matrix.drawPixel(x,y,c);} }
    Matrix.show(); delay(20); t+=SPEED; }
  // 色相を進めず固定
  for(int b=gMotionBrightness; b>=0; b-=2){ Matrix.setBrightness(b); Matrix.show(); delay(18);} Matrix.fillScreen(0); Matrix.show();
}

void Text_PlayOnce(const char* text,uint16_t frame_delay_ms){
  Matrix.setBrightness(gTextBrightness);
  int textWidth=getStringWidth(text); MatrixWidth=Matrix.width();
  while(MatrixWidth >= -textWidth){ Matrix.fillScreen(0); Matrix.setCursor(MatrixWidth,0); Matrix.print(text); Matrix.show(); MatrixWidth--; delay(frame_delay_ms);} }

// 非ブロッキング レーダー（色相固定）
static float sRadarAngleDeg=0.f; static bool sRadarActive=false; const float RADAR_SPEED=2.5f; const float BW_F=0.8f; const float BW_B=0.05f; const uint8_t FADE=10; // const uint8_t HSTEP_IDLE=24; // 未使用
void Radar_InitIdle(){ sRadarAngleDeg=0.f; Matrix.fillScreen(0); Matrix.setBrightness(gMotionBrightness); Matrix.show(); sRadarActive=true; }
void Radar_IdleStep(bool doShow){ if(!sRadarActive) return; for(int i=0;i<Matrix.width()*Matrix.height();++i){ uint32_t col=Matrix.getPixelColor(i); if(col){ uint8_t r=(col>>16)&0xFF,g=(col>>8)&0xFF,b=col&0xFF; r=(r<=FADE)?0:r-FADE; g=(g<=FADE)?0:g-FADE; b=(b<=FADE)?0:b-FADE; Matrix.setPixelColor(i,Matrix.Color(r,g,b)); }} const float cx=(Matrix.width()-1)*0.5f, cy=(Matrix.height()-1)*0.5f; float rad=sRadarAngleDeg*(float)M_PI/180.f; for(uint8_t y=0;y<Matrix.height();++y){ for(uint8_t x=0;x<Matrix.width();++x){ float dx=x-cx,dy=y-cy; float pr=atan2f(dy,dx); float diff=rad-pr; while(diff>M_PI) diff-=2.f*M_PI; while(diff<-M_PI) diff+=2.f*M_PI; float bw=(diff>0)?BW_F:BW_B; float br=expf(-(diff*diff)/(2.f*bw*bw)); if(br>0.05f){ uint8_t V=gamma8(br); uint16_t c=ColorHSV8(gMotionHue,255,V); Matrix.drawPixel(x,y,c);} }} if(doShow) Matrix.show(); sRadarAngleDeg+=RADAR_SPEED; if(sRadarAngleDeg>=360.f){ sRadarAngleDeg-=360.f; /* 色相固定: 変化させない */ }}
