#include <Arduino.h>
#include <NimBLEDevice.h>
#include "Display_text.h"
#include "Motion.h"

#define BOOT_BTN_PIN 0

static const NimBLEUUID kServiceUUID((uint16_t)0xFFF0);
static const char* kMessageBase = "ThankYou!";
static const int kRssiThreshold = -50; // これ未満(RSSI値が小さい=遠い)は無視

NimBLEAdvertising* gAdv = nullptr;
std::string gMyMsg;
NimBLEAddress gMyAddr;
NimBLEScan* gScanner = nullptr;

volatile bool gHasPending = false;
std::string gPendingMsg;

void startAdvertising(const std::string& myMsg) {
  NimBLEAdvertisementData ad;
  ad.setFlags(0x06);
  ad.setServiceData(kServiceUUID, myMsg);
  ad.addServiceUUID(kServiceUUID);
  gAdv->setAdvertisementData(ad);
  gAdv->start();
  Serial.print("[ADV] start: ");
  Serial.println(myMsg.c_str());
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    if (dev->getAddress() == gMyAddr) return;
    int rssi = dev->getRSSI();
    if (rssi < kRssiThreshold) {
      // Serial.printf("[SKIP] %s RSSI=%d<th %d\n", dev->getAddress().toString().c_str(), rssi, kRssiThreshold);
      return; // 閾値未満は無視
    }
    std::string data = dev->getServiceData(kServiceUUID);
    if (data.empty()) return;
    std::string display = data;
    size_t pos = display.find('#');
    if (pos != std::string::npos) display.erase(pos);
    if (!gHasPending) {
      gPendingMsg = display;
      gHasPending = true;
    }
    Serial.print("[RX] from ");
    Serial.print(dev->getAddress().toString().c_str());
    Serial.print(" RSSI=");
    Serial.print(rssi);
    Serial.print(" dBm  msg=\"");
    Serial.println(data.c_str());
  }
} gScanCallbacks;

void setup() {
  // 任意: 初期パラメータ（必要に応じ調整）

  delay(3000);
  Matrix_SetTextBrightness(1);    // 文字明るさ
  Matrix_SetMotionBrightness(1);  // モーション明るさ


  Serial.begin(115200);
  //while (!Serial) { delay(10); }
  Serial.println("\n== Dual Role: Beacon + Scanner ==");
  Matrix_Init();
  Radar_InitIdle();
  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);

  NimBLEDevice::init("");
  NimBLEDevice::setPower(3);
  gMyAddr = NimBLEDevice::getAddress();
  {
    std::string mac = gMyAddr.toString();
    std::string tail = mac.substr(9);
    std::string id;
    for (char ch : tail) if (ch != ':') id += (char)toupper(ch);
    gMyMsg = std::string(kMessageBase) + "#" + id;
  }
  gAdv = NimBLEDevice::getAdvertising();
  startAdvertising(gMyMsg);

  gScanner = NimBLEDevice::getScan();
  gScanner->setScanCallbacks(&gScanCallbacks, false);
  gScanner->setActiveScan(true);
  gScanner->setInterval(100);
  gScanner->setWindow(100);

  Serial.print("My MAC: "); Serial.println(gMyAddr.toString().c_str());
  Serial.print("My MSG: "); Serial.println(gMyMsg.c_str());
  Serial.println("Scanning + Advertising...");

  Ripple_PlayOnce(600); // 起動演出（モーション色相/明るさ使用）
  Radar_InitIdle(); 
}

void loop() {
  static bool prev = HIGH;
  static uint32_t lastMs = 0;
  bool curr = digitalRead(BOOT_BTN_PIN);
  uint32_t now = millis();

  // ボタン押下: 手動でRipple再生 → レーダー復帰
  if (prev == HIGH && curr == LOW && (now - lastMs) > 200) {
    if (gScanner) gScanner->stop();
    Ripple_PlayOnce(600);
    Radar_InitIdle();
    if (gScanner) gScanner->start(5, false);
    lastMs = now;
  }
  prev = curr;

  // 受信イベント処理: Ripple → テキスト → レーダー再開
  if (gHasPending) {
    if (gScanner) gScanner->stop();
    Ripple_PlayOnce(600);
    Text_PlayOnce(gPendingMsg.c_str(), 100);
    gHasPending = false;
    Radar_InitIdle();
    if (gScanner) gScanner->start(5, false);
  }

  // スキャン継続（終わっていたら再始動）
  if (gScanner && !gScanner->isScanning()) {
    gScanner->start(5, false);
    gScanner->clearResults();
  }

  // アイドル中レーダー1フレーム描画
  Radar_IdleStep(true);

  delay(16); // 約60fps
}
