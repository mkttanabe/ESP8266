#include <ESP8266WiFi.h>
#include <EspHttpsClient.h>
#include "OneButtonMailer.h"

extern "C" {
#include "ip_addr.h"
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

  // create request
  char bufResponse[512];
  int len_body = strlen(BODY);
  int len = strlen(HEADER) + len_body + 4; // for Content-Length field
  char *pRequest = (char*)malloc(len);
  sprintf(pRequest, HEADER BODY, len_body);
  Serial.printf("\r\n<<request>>\r\n[%s]\r\n", pRequest);

  // request to SendGrid service
  int sts = doRequest(HOSTNAME, pRequest, bufResponse, sizeof(bufResponse), true);
  free(pRequest);

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
