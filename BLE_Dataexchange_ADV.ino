#include <Arduino.h>
#include <NimBLEDevice.h>
#include "WS_Flow.h"

#define BOOT_BTN_PIN 0  /////////////////////////////////////////////////////////////////

// 共有のService UUID（任意の16-bitでOK。必要なら128-bitに変更可）
static const NimBLEUUID kServiceUUID((uint16_t)0xFFF0);

// 自分の基本メッセージ（あとで末尾に短いIDを付ける）
static const char* kMessageBase = "Thankyou!";

// 広告（送信）関連
NimBLEAdvertising* gAdv = nullptr;
std::string gMyMsg;
NimBLEAddress gMyAddr;

// スキャン（受信）関連
NimBLEScan* gScanner = nullptr;

// 追加: 受信ペンディング管理
volatile bool gHasPending = false;
std::string gPendingMsg;

// 自分のアドバタイズを作成して開始
void startAdvertising(const std::string& myMsg) {
  NimBLEAdvertisementData ad;
  ad.setFlags(0x06);  // 一般発見可能・BR/EDR非対応
  ad.setServiceData(kServiceUUID, myMsg);
  ad.addServiceUUID(kServiceUUID);

  gAdv->setAdvertisementData(ad);
  gAdv->start();  // 垂れ流し（止めない）
  Serial.print("[ADV] start: ");
  Serial.println(myMsg.c_str());
}

// 受信コールバック
class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    // 自分自身の広告は無視
    if (dev->getAddress() == gMyAddr) return;

    // 指定UUIDのService Dataを取得
    std::string data = dev->getServiceData(kServiceUUID);
    if (data.empty()) return;

    // 表示用に '#' 以降（ID部分）を削除
    std::string display = data;
    size_t pos = display.find('#');
    if (pos != std::string::npos) {
      display.erase(pos);
    }

    // コールバックでは保存だけ（多重受付を抑制）
    if (!gHasPending) {
      gPendingMsg = display;  // マトリクス表示はIDなし
      gHasPending = true;
    }

    // デバッグ表示（フル文字列＝ID付きのまま）
    Serial.print("[RX] from ");
    Serial.print(dev->getAddress().toString().c_str());
    Serial.print(" RSSI=");
    Serial.print(dev->getRSSI());
    Serial.print(" dBm  msg=\"");
    Serial.println(data.c_str());
  }
} gScanCallbacks;


void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("\n== Dual Role: Beacon + Scanner ==");
  Matrix_Init();

  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);  ///////////////////////////////////////////////////////////////////////


  // ---- NimBLE 初期化 ----
  NimBLEDevice::init("");     // デバイス名は空（最小）
  NimBLEDevice::setPower(3);  // 全送信出力を +3 dBm に（必要に応じて調整）

  gMyAddr = NimBLEDevice::getAddress();

  // 自分の短いID（MAC下位3バイトなど）をメッセージに付与
  {
    std::string mac = gMyAddr.toString();
    std::string tail = mac.substr(9);
    std::string id;
    for (char ch : tail)
      if (ch != ':') id += (char)toupper(ch);
    gMyMsg = std::string(kMessageBase) + "#" + id;
  }

  // ---- 広告開始（送信）----
  gAdv = NimBLEDevice::getAdvertising();
  startAdvertising(gMyMsg);

  // ---- スキャン設定（受信）----
  gScanner = NimBLEDevice::getScan();
  gScanner->setScanCallbacks(&gScanCallbacks, false);
  gScanner->setActiveScan(true);
  gScanner->setInterval(100);
  gScanner->setWindow(100);

  Serial.print("My MAC: ");
  Serial.println(gMyAddr.toString().c_str());
  Serial.print("My MSG: ");
  Serial.println(gMyMsg.c_str());
  Serial.println("Scanning + Advertising...");
}

void loop() {
  static bool prev = HIGH;
  static uint32_t lastMs = 0;
  bool curr = digitalRead(BOOT_BTN_PIN);
  uint32_t now = millis();
  if (prev == HIGH && curr == LOW && (now - lastMs) > 200) {  // 押下エッジ＋デバウンス
    if (gScanner) gScanner->stop();
    Ripple_PlayOnce(600);  // 約600msのリップル
    lastMs = now;
  }
  prev = curr;


  // 受信があれば、スキャンを停止して Ripple→テキストを最後まで表示
  if (gHasPending) {
    if (gScanner) gScanner->stop();
    Ripple_PlayOnce(600);
    Text_PlayOnce(gPendingMsg.c_str(), 100);  // 40ms/フレーム（調整可）
    gHasPending = false;
  }

  // 広告は常時出しっぱなし、スキャンを回す
  gScanner->start(5 /*秒*/, false /*is_continue*/);
  gScanner->clearResults();
  delay(200);
}
