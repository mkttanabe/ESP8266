#include <ESP8266WiFi.h>
#include <EspHttpsClient.h>
#include "OneButtonMailer_IFTTT.h"

extern "C" {
#include "user_interface.h"
}

#define pinLED 4

void setup() {
  Serial.begin(9600);
  Serial.println("setup");

  // turn LED on
  pinMode(pinLED, OUTPUT);  
  digitalWrite(pinLED, HIGH);

  // connect to AP
  WiFi.begin(MYSSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  Serial.printf("\r\n<<request>>\r\n[%s]\r\n", HEADER);
  char bufResponse[512];

  // request to IFTTT Maker channel
  int sts = doRequest(HOSTNAME, HEADER, bufResponse, sizeof(bufResponse), true);

  // result
  Serial.printf("doRequest: result=%d\r\n", sts);
  if (sts == 0) {
    Serial.printf("\r\n<<response>>\r\n[%s]\r\n", bufResponse);
  }
  // turn LED off
  digitalWrite(pinLED, LOW);

  // into deep sleep
  system_deep_sleep_set_option(0);
  system_deep_sleep(0); // forever
}

void loop() {
}
