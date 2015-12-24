#include <pubnub.h>
#include <ESP8266WiFi.h>
#include <SimpleTimer.h>

#define MYSSID         "ssid"
#define PASS           "pass"
#define PUBNUB_CHANNEL "Channel-3lo░░░░░░"
#define PUBNUB_PUBKEY  "pub-c-1f22eae4░░░░░░░░░░░░░░░░░░░░░░░"
#define PUBNUB_SUBKEY  "sub-c-3afea402░░░░░░░░░░░░░░░░░░░░░░░"
#define MYID           "device01"

#define NUM_LED  4

#define R  0
#define G  1
#define B  2
#define Y  3

#define CMD_ON      1
#define CMD_OFF     0
#define CMD_BLINK  -1

static int8_t LEDpins[NUM_LED] = {4, 5, 12, 13};
static int8_t LEDcmd[NUM_LED];
static SimpleTimer timer;

void setup() {
  Serial.begin(9600);
  Serial.printf("setup");
#ifdef USE_SSL
  Serial.printf("(SSL)");
#endif

  for (int i = 0; i < NUM_LED; i++) {
    pinMode(LEDpins[i], OUTPUT);
  }
  cmdLED("blink");

  pubnub_init(PUBNUB_PUBKEY, PUBNUB_SUBKEY);

  // connect to AP
  WiFi.begin(MYSSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    cmdLED("blink");
    delay(200);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  pubnub_connect();
  bool sts;
  do {
    cmdLED("blink");
    sts = pubnub_subscribe(PUBNUB_CHANNEL, subscribeCB);
    Serial.printf("pubnub_subscribe sts=%d\r\n", sts);
    delay(200);
  } while (!sts);

  cmdLED("off");
  setTimer();
}

void loop() {
  timer.run();
}

// callback function for pubnub_publish()
void publishCB(char *msg)
{
  Serial.printf("app: publishCB\r\n");
  // ready to receive the next message 
  pubnub_subscribe(PUBNUB_CHANNEL, subscribeCB);
}

// callback function for pubnub_subscribe()
void subscribeCB(char *msg)
{
  Serial.printf("app: subscribeCB data=[%s]\r\n", msg);
  // ignore self-sent messages
  if (strstr(msg, MYID) || strlen(msg) == 0) {
    pubnub_subscribe(PUBNUB_CHANNEL, subscribeCB);
  } else {
    // handle LED command
    handleMessage(msg);
    // send current status
    char buf[48];
    os_sprintf(buf, MYID ": R%d G%d B%d Y%d", LEDcmd[R], LEDcmd[G], LEDcmd[B], LEDcmd[Y]);
    bool sts = pubnub_publish(PUBNUB_CHANNEL, buf, publishCB);
    Serial.printf("pubnub_publish sts=%d\r\n", sts);
  }
}

static void setTimer()
{
  timer.setInterval(800, setLED);
}

static void setLED()
{
  boolean LEDsts[NUM_LED];
  int8_t blinking[NUM_LED];
  int8_t numBlink = 0;
  int8_t pin, cmd, b_cur = -1, b_next = -1;

  for (int i = 0; i < NUM_LED; i++) {
    pin = LEDpins[i];
    cmd = LEDcmd[i];
    LEDsts[i] = digitalRead(pin);
    if (cmd == CMD_BLINK) {
      if (LEDsts[i] == HIGH) {
        b_cur = i;
      }
      blinking[numBlink++] = i;
    } else if (cmd == CMD_OFF && LEDsts[i] == HIGH) {
      digitalWrite(pin, LOW);
    } else if (cmd == CMD_ON && LEDsts[i] == LOW) {
      digitalWrite(pin, HIGH);
    }
  }
  // for blinking
  if (numBlink > 0) {
    // not exist LED in lighting
    if (b_cur == -1) {
      b_next = blinking[0];
    }
    else {
      for (int i = 0; i < numBlink; i++) {
        if (blinking[i] == b_cur) {
          // turn off the current LED
          digitalWrite(LEDpins[blinking[i]], LOW);
          // determine the next LED to turn on
          if (numBlink == 1) {
            break;
          }
          if (i < numBlink - 1) {
            b_next = blinking[i + 1];
          } else {
            b_next = blinking[0];
          }
          break;
        }
      }
    }
    if (b_next != -1) {
      digitalWrite(LEDpins[b_next], HIGH);
    }
  }
}

static void handleMessage(char *msg)
{
  char *p, c;
  int8_t cmd;
  if ((p = strstr(msg, "on"))) {
    cmd = CMD_ON;
    c = *(p + 2);
  } else if ((p = strstr(msg, "off"))) {
    cmd = CMD_OFF;
    c = *(p + 3);
  } else if ((p = strstr(msg, "blink"))) {
    cmd = CMD_BLINK;
    c = *(p + 5);
  } else {
    return;
  }
  if (c == 'R') {
    LEDcmd[R] = cmd;
  } else if (c == 'G') {
    LEDcmd[G] = cmd;
  } else if (c == 'B') {
    LEDcmd[B] = cmd;
  } else if (c == 'Y') {
    LEDcmd[Y] = cmd;
  } else {
    memset(LEDcmd, cmd, sizeof(LEDcmd));
  }
}

static void cmdLED(char *cmd)
{
  handleMessage(cmd);
  setLED();
}

