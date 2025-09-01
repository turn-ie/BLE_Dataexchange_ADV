#include <Arduino.h>
#include <NimBLEDevice.h>
#include "WS_Flow.h"

// 共有のService UUID（任意の16-bitでOK。必要なら128-bitに変更可）
static const NimBLEUUID kServiceUUID((uint16_t)0xFFF0);

// 自分の基本メッセージ（あとで末尾に短いIDを付ける）
static const char* kMessageBase = "hello-esp32";

// 広告（送信）関連
NimBLEAdvertising* gAdv = nullptr;
std::string gMyMsg;
NimBLEAddress gMyAddr;

// スキャン（受信）関連
NimBLEScan* gScanner = nullptr;

// 自分のアドバタイズを作成して開始
void startAdvertising(const std::string& myMsg) {
  NimBLEAdvertisementData ad;
  ad.setFlags(0x06); // 一般発見可能・BR/EDR非対応
  ad.setServiceData(kServiceUUID, myMsg);
  ad.addServiceUUID(kServiceUUID);

  gAdv->setAdvertisementData(ad);
  gAdv->start();  // 垂れ流し（止めない）
  Serial.print("[ADV] start: "); Serial.println(myMsg.c_str());
}

// 受信コールバック
// 受信コールバック
class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    // 自分自身の広告は無視
    if (dev->getAddress() == gMyAddr) return;

    // 指定UUIDのService Dataを取得
    std::string data = dev->getServiceData(kServiceUUID);
    if (data.empty()) return;

    // ここでText_Flowに渡す
    Text_Flow((char*)data.c_str());


    // --- デバッグ表示が必要なら残す ---
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

  // ---- NimBLE 初期化 ----
  NimBLEDevice::init("");          // デバイス名は空（最小）
  NimBLEDevice::setPower(3);       // 全送信出力を +3 dBm に（必要に応じて調整）

  gMyAddr = NimBLEDevice::getAddress();

  // 自分の短いID（MAC下位3バイトなど）をメッセージに付与
  // 例: "hello-esp32#A1B2C3"
  {
    std::string mac = gMyAddr.toString(); // 例 "aa:bb:cc:dd:ee:ff"
    // 下位3バイトだけ抜き出してID化
    // mac.substr(9) -> "dd:ee:ff" を "DDEEFF" に
    std::string tail = mac.substr(9); // "dd:ee:ff"
    std::string id;
    for (char ch : tail) if (ch != ':') id += (char)toupper(ch);
    gMyMsg = std::string(kMessageBase) + "#" + id;
  }

  // ---- 広告開始（送信）----
  gAdv = NimBLEDevice::getAdvertising();
  startAdvertising(gMyMsg);

  // ---- スキャン設定（受信）----
  gScanner = NimBLEDevice::getScan();
  gScanner->setScanCallbacks(&gScanCallbacks, false);
  gScanner->setActiveScan(true); // スキャンレスポンスも取得
  gScanner->setInterval(100);    // 連続受信気味
  gScanner->setWindow(100);

  Serial.print("My MAC: "); Serial.println(gMyAddr.toString().c_str());
  Serial.print("My MSG: "); Serial.println(gMyMsg.c_str());
  Serial.println("Scanning + Advertising...");
}

void loop() {
  // 広告は常時出しっぱなし
  // スキャンは数秒ごとに回して結果はコールバックで処理
  gScanner->start(5 /*秒*/, false /*is_continue*/);
  gScanner->clearResults();
  delay(200); // 少し間を空けて再スキャン
}
