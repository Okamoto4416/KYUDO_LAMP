# NetworkManagerのテスト

## 2025/06/01 5:21 Connectedまで確認した。

- 日付：2025/06/01 5:21

- 内容：二台接続してConnected状態まで到達できるか

- 結果：成功（以下時系列にやったこと)
  
  - 二台起動：connected到達確認した。
  
  - APoff: unstable -> discoveringPeerのちconnectingAPへの遷移を確認
  
  - APon:connectingAP->->connectedを確認
  
  - 一台off: connected -> unstalbe -> discoveringPeerを確認
  
  - 一台再on :discoveringPeer -> -> connectedを確認

- 次回以降：
  
  - 時間同期の確認
  
  - jsonパケットちゃんと送れるか確認
  
  - 本番距離でちゃんとやれるか
