# BLE_Dataexchange_ADV

ESP32-S3 同士でBLEアドバタイズのService Dataをやり取りし、受信時にWS2812 8x8マトリクスへ「リップル → テキストスクロール」を表示するサンプル。

- リップル: 1サイクルのみ再生し、最後はフェードアウトして完全消灯
- テキスト: 受信メッセージから末尾のID（`#`以降）を除去して表示
- 受信フロー: 受信検知 → スキャン停止 → リップル1回 → 全文スクロール完了 → スキャン再開
- BOOTボタン（GPIO0）押下: リップル1回

## バージョン/環境（検証時）
- Arduino CLI: 1.3.0
- ボードパッケージ: ESP32 by Espressif Systems 2.0.14（最新 3.3 あり）
- FQBN: `esp32:esp32:gen4-ESP32-S3R8n16`
- ライブラリ:
  - NimBLE-Arduino 2.3.4（`#include <NimBLEDevice.h>`）
  - Adafruit GFX Library 1.12.1
  - Adafruit NeoPixel 1.15.1
  - Adafruit NeoMatrix 1.3.3
- OS: macOS、シェル: zsh

## ハードウェア
- ボード: 4D Systems gen4-ESP32 16MB ESP32-S3R8n16（相当）
- LEDマトリクス: WS2812B 8x8（800kHz）
- 配線:
  - DATA IN ← ESP32 GPIO14（`RGB_Control_PIN = 14`）
  - 5V/GND を安定給電（LEDの電流に注意）
  - BOOTボタン（GPIO0）: プルアップ（INPUT_PULLUP）。GNDへ落とすと押下

## ボード設定の要点
- USB CDC On Boot: Enable（USBCDC boot Enable）
- シリアルポート: `/dev/tty.usbmodemXXXX` を推奨（`/dev/cu.*`より安定）
- そのほかはデフォルトで可（必要に応じてPSRAMや速度は環境に合わせる）

## プロジェクト構成
- `BLE_Dataexchange_ADV.ino`: メイン（BLE広告/スキャン、受信フロー制御、BOOTボタン）
- `WS_Flow.h` / `WS_Flow.cpp`: マトリクス表示（初期化、リップル、テキスト表示）

## 動作仕様
- 受信メッセージ書式: `本文#ID`（例: `Thankyou!#A1B2C3`）
  - 表示時は `#` 以降（ID）を除去。シリアルログにはフルを出力
- リップル: 1周期のみ。ガンマ補正・段階レベルで発色し、終了時は明るさを0へフェードアウト
- スクロール: 文字幅計算に基づき全文スクロール完了までブロッキング表示
- スキャン: アニメーション中は停止。終了後に再開
- BOOTボタン: 立ち下がり検出でリップル1回

## カスタマイズ（主な変更点）
- 明るさ:
  - 通常: `Matrix.setBrightness(1)`（`WS_Flow.cpp` の `Matrix_Init()`）
  - リップル中: 一時的に 22 に設定 → 終了時に 0 までフェード → 元の値に復帰
- 色:
  - デフォルトは `colors[0]`（`Matrix_Init()`）。`Matrix_SetTextColorIndex()`で変更可
- スクロール速度:
  - `Text_PlayOnce(const char* text, uint16_t frameDelayMs)` の `frameDelayMs` を調整
- マトリクス配列・向き:
  - `Adafruit_NeoMatrix` のフラグ（TOP/RIGHT, COLUMNS/ROWS, PROGRESSIVE/ZIGZAG）を実機に合わせて調整
- ピン:
  - `WS_Flow.h` の `RGB_Control_PIN`（既定 14）

## ビルド/書き込み
Arduino IDE でも Arduino CLI でも可。CLI例:

```sh
# コア/ライブラリ（必要に応じて）
arduino-cli core install esp32:esp32
arduino-cli lib install "NimBLE-Arduino" "Adafruit GFX Library" "Adafruit NeoPixel" "Adafruit NeoMatrix"

# コンパイル
arduino-cli compile -b esp32:esp32:gen4-ESP32-S3R8n16

# ポートは /dev/tty.usbmodemXXXX を指定
arduino-cli upload -p /dev/tty.usbmodem11401 -b esp32:esp32:gen4-ESP32-S3R8n16
```

ヒント:
- macOSでは `/dev/tty.usbmodem*` が安定。見つからない場合は `arduino-cli board list` で確認
- 0バイトのbinが生成された場合はクリーン後に再コンパイル（ビルド生成物を削除）

## トラブルシューティング
- 表示が真っ白/まぶしい:
  - `Matrix_Init()` が一度だけ呼ばれているか確認
  - 明るさ（通常1、リップル22）を下げる
  - マトリクス向き/配列フラグが実機と一致しているか確認
- 受信直後にテキストが途切れる/混ざる:
  - BLEコールバックで描画しない。`loop()`でフラグを見てリップル→スクロールを順次実行
- アップロードできない/ポートがない:
  - `/dev/tty.usbmodem*` を使用。必要ならBOOT/EN手順（ボードに依存）
- ビルドが通るのにbinが空:
  - 一度クリーン（`.build` など）して再ビルド

## 既知の制限
- アニメーション中はブロッキングのため、スキャンを一時停止
- メッセージはキューイングしていない（受信中に新規は1件だけ保留）

## 変更履歴
- 2025-09-03: README整備、受信時のID非表示、リップル1サイクル＆フェードアウト仕様を記録

## ライセンス
- TBD

