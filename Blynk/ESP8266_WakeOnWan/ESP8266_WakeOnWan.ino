/******************************************************************************
 *
 * ESP8266 モジュールと Blynk で遠隔の PC を Wake On Wan
 *
 * 2015 KLab Inc.
 *
*******************************************************************************/

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WiFiUdp.h>
#include <SimpleTimer.h>

extern "C" {
#include "user_interface.h"
#include "ping.h"
}

#define MAGIC_PACKET_LENGTH 102
#define PORT_WAKEONLAN 9

WiFiUDP udp;
// ping テストの対象とする IP アドレス
const char* ping_ip = "10.10.0.3";

// Wake On Wan 対象 PC の NIC の MAC アドレス
byte macAddr[6] = {0xd5, 0x74, 0xD8, 0x21, 0x28, 0x32};
// 当該 LAN のブロードキャストアドレス
IPAddress bcastAddr(10, 10, 0, 255);
byte magicPacket[MAGIC_PACKET_LENGTH];
unsigned int localPort = 8761;

SimpleTimer timer;
unsigned long timeRestartPressed = 0;
int stsPing, countPing = -1;
struct ping_option popt;

int pinLED = 4;     // Digital  4 - LED 
int pinAC = 12;     // Digital 12 - リレーケーブル

// アプリ側で使用する仮想ピン番号
#define APP_BUTTON_RESTART    V0
#define APP_BUTTON_WAKEONLAN  V1
#define APP_BUTTON_PING       V2
#define APP_DISPLAY_PING_OK   V11
#define APP_DISPLAY_PING_NG   V12
#define APP_DISPLAY_TICK      V30

// Blynk App Auth Token
char auth[] = "5b1c7xxxxxxxxxxxxxxxxxxxxxxxxxxx";

void setup()
{
  Serial.begin(9600);
  pinMode(pinLED, OUTPUT);  
  pinMode(pinAC, OUTPUT);
  Blynk.begin(auth, "ssid", "pass");
  if (udp.begin(localPort) == 1) {
    BLYNK_LOG("udp begin OK");
    buildMagicPacket();
  }
  doConnect();
  Blynk.virtualWrite(APP_DISPLAY_PING_OK, 0);
  Blynk.virtualWrite(APP_DISPLAY_PING_NG, 0);
  Blynk.virtualWrite(APP_DISPLAY_TICK, 0);
  setTimer();
}

void loop()
{
  Blynk.run();
  timer.run();
}

// タイマー処理をセット
void setTimer()
{
  timer.setInterval(1000, setPingResult);
  timer.setInterval(2000, tick);
}

// 稼働状況確認用
void tick()
{
  // デバイス上の LED を点滅させる
  digitalWrite(pinLED, !digitalRead(pinLED));
  // 稼働継続秒数をアプリ側へ通知
  Blynk.virtualWrite(APP_DISPLAY_TICK, millis() / 1000);
}

// Blynk サーバへの接続
void doConnect()
{
  BLYNK_LOG("doConnect start");
  bool sts = false;
  Blynk.disconnect();
  sts = Blynk.connect(10000);
  BLYNK_LOG("Blynk.connect=%d", sts);
  if (!sts) {
    BLYNK_LOG("connect timeout. restarting..");
    doRestart();
  }
}

// 本デバイスを再起動
void doRestart()
{
  digitalWrite(pinLED, LOW);
  Blynk.virtualWrite(APP_DISPLAY_TICK, 0);
  system_restart(); // ESP8266 API
}

// マジックパケットを生成
void buildMagicPacket()
{
  memset(magicPacket, 0xFF, 6);
  for (int i = 0; i < 16; i++) {
    int ofs = i * sizeof(macAddr) + 6;
    memcpy(&magicPacket[ofs], macAddr, sizeof(macAddr));
  }
}

// 所定の PC への ping 状況をアプリの LED へ反映
void setPingResult()
{
  //BLYNK_LOG("pinging=%d", pinging);
  if (countPing != -1) {
      Blynk.virtualWrite(stsPing ? APP_DISPLAY_PING_OK : APP_DISPLAY_PING_NG, countPing);
  }
}

// ping 結果受信時コールバック
void cb_ping_recv(void *arg, void *pdata)
{
  countPing++;
  stsPing = 0;
  struct ping_resp *ping_resp = (struct  ping_resp *)pdata;
  struct ping_option *ping_opt = (struct ping_option *)arg;
  if (ping_resp->ping_err == -1) {
    BLYNK_LOG("ping failed");
  } else {
    stsPing = 1;
    BLYNK_LOG("ping recv: byte = %d, time = %d ms", ping_resp->bytes, ping_resp->resp_time);
  }
}

// ping 終了時コールバック
void cb_ping_sent(void *arg, void *pdata)
{
  countPing = -1;
  BLYNK_LOG("ping finished");
}

// ping 実行
void doPing(const char *targetIpAddress)
{
  struct ping_option *ping_opt = &popt;
  ping_opt->count = 6;
  ping_opt->coarse_time = 1;  // interval
  ping_opt->ip = ipaddr_addr(targetIpAddress);
  ping_regist_recv(ping_opt, cb_ping_recv);
  ping_regist_sent(ping_opt, cb_ping_sent);
  ping_start(ping_opt);
}

// アプリの RESTART ボタンハンドラ
BLYNK_WRITE(APP_BUTTON_RESTART)
{
  //BLYNK_LOG("AppButtonRestart: value=%d", param.asInt());
  if (param.asInt() == 1) {
    timeRestartPressed = millis();
  } else {
    if (timeRestartPressed <= 0) {
      return;
    }
    unsigned long past = millis() - timeRestartPressed;
    timeRestartPressed = 0;
    // 3秒以上 RESTART ボタン押下～リリースでデバイスをリセット
    if (past >= 3000) {
      BLYNK_LOG("AppButtonRestart: restarting..");
      doRestart();
    }
  }
}

// アプリの BOOT PC ボタンハンドラ
BLYNK_WRITE(APP_BUTTON_WAKEONLAN)
{
  //BLYNK_LOG("AppButtonWakeOnLan: value=%d", param.asInt());
  udp.beginPacket(bcastAddr, PORT_WAKEONLAN);
  udp.write(magicPacket, MAGIC_PACKET_LENGTH);
  udp.endPacket();
}

// アプリの PING ボタンハンドラ
BLYNK_WRITE(APP_BUTTON_PING)
{
  if (countPing != -1) {
    return;
  } else {
    BLYNK_LOG("start pinging");
    countPing = 0;
    Blynk.virtualWrite(APP_DISPLAY_PING_OK, 0);
    Blynk.virtualWrite(APP_DISPLAY_PING_NG, 0);
    doPing(ping_ip);
  }
}

