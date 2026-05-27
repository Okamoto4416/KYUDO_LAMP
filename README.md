# 気持ち

## 設計・定義

- 区分
  
  - 基盤
    
    - ネットワーク・WiFi・UDP通信
    
    - 点滅出力(パターン点滅)
  
  - 機能
    
    - 通信計測
    
    - 状態表示LED
    
    - ランプ
    
    - ランプ側ランプ制御ボタン
    
    - リモコン側ランプ制御ボタン
    
    - 状態共有

### ネットワーク・WiFi・UDP通信

実装：NetworkMngr_t

#### 望む機能

- 接続が切れても自動で再接続

- 通信状態認識

- 時間同期

#### 通信状態の定義

- off (Wifi機能がoff)

- connectingAP (APへ接続試行中)
  
  - 状態：
    
    - APへ接続できていない状態で試行している
  
  - 遷移：
    
    - APに接続できたらdiscoveringPeerへ

- discoveringPeer (相手発見待ち)
  
  - 状態：
    
    - connectingAP成功からの遷移先
    
    - discovered条件満たさない(過去5秒Peerからの通信なし)
  
  - 遷移：
    
    - 通信来たらunstableへ
    
    - APへの接続切れたらconnectingAPへ

- unstable (通信可能だが不安定)
  
  - 状態：
    
    - discovered条件満たす
    
    - connected条件満た<u>さない</u>。
  
  - 遷移：
    
    - connectedの条件を満たしたらconnectedへ
    
    - APへの接続切れたらconnectingAPへ
    
    - discovered条件満たさなかったらdiscoveringPeer

- connected (通信良好)
  
  - 状態：
    
    - discovered条件満たす
    
    - connected条件満たす
  
  - 遷移：
    
    - connectedの条件を満たさなくなったらunstableへ
    
    - APへの接続切れたらconnectingAPへ
    
    - discovered条件満たさなかったらdiscoveringPeer

- error (その他エラー)

- 条件：
  
  - connected条件：通信良好な条件
    
    - パケット到着率条件：1秒のmiss率20%以下
    
    - または
    
    - 直近5パケットのうち4パケットack到着済み
  
  - discoverd条件：Peerが見つかっている条件
    
    - 過去4.8秒の間に計測パケットのackを受信している

### 点滅出力(パターン点滅)

実装：PatternBlinker8bit

#### 動作

8bit整数でパターンを与えて左の桁から周期8で点滅させていく。

時間の周期は1sに設定している。

### 通信計測

### 状態表示LED

### ランプ

### ランプ側ランプ制御ボタン

### リモコン側ランプ制御ボタン

### 状態共有

## 参考

- WiFiについて
  
  - 切断時の再接続方法について
    
    - [[ESP32]Wifi切断時に自動再接続する方法 with Arduino IDE | farmsoft](https://www.farmsoft.jp/86/)
  
  - WiFiについて
    
    - [ESP32でのWi-Fi接続 | Lang-ship](https://lang-ship.com/blog/work/esp32-wi-fi/)
  
  - 5秒リトライについて
    
    - [ESP32 WiFiにつながらないとき #ESP32 - Qiita](https://qiita.com/nak435/items/33f05988575f8a61b2a3)
  
  - リファレンス？
    
    - https://docs.arduino.cc/language-reference/en/functions/wifi/overview/

- WiFiudpについて
  
  - リファレンス？
    
    - https://docs.arduino.cc/language-reference/en/functions/wifi/udp/

- ArduinoJSONについて
  
  - 使い方
    
    - [ArduinoでJsonを生成、解析する #C++ - Qiita](https://qiita.com/BB-KING777/items/a4c0d0116ef8a0faa806)
  
  - リファレンス
    
    - https://arduinojson.org/v7/api/json/

- 
