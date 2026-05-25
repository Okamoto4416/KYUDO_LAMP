/*
ルータをAPとして利用する場合の設定
*/
#include <WiFi.h>
constexpr char ssid[] = "el2g-6cec04";
constexpr char pass[] = "402fng1001";

// constexpr char STA_IP[] = "192.168.0.50";
// constexpr char AP_IP[] = "192.168.0.1";
// constexpr char GATEWAY[] ="192.168.0.1";
// constexpr char SUBNET[] ="255.255.255.0";

const IPAddress ap_ip(192, 168, 2, 1);
const IPAddress lamp_ip(192, 168, 2, 50);    // ランプ側のIPアドレス
const IPAddress remote_ip(192, 168, 2, 60); // リモコン側のIPアドレス
const IPAddress gateway(192, 168, 2, 1);
const IPAddress subnet(255, 255, 255, 0);