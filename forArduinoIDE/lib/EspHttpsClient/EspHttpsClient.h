#ifndef EspHttpClient_H
#define EspHttpClient_H
//
// EspHttpsClient.h
//
// 2015 KLab Inc.
//
// for "Arduino IDE for ESP8266"
// https://github.com/esp8266/Arduino
//

#include "Arduino.h"

int doRequest(const char *hostname,
              const char *request,
              char *respbuf,
              int resbuf_len,
              boolean use_ssl = false,
              short port = 0,
              boolean ssl_verify = false,
              uint16 flash_sector_of_cert = 0x00);

/*
 * Usage of doRequest()
 *
 * [Parameters]
 *
 *  hostname : target hostname
 *           : e.g. "www.example.com"
 *
 *  request  : request message
 *           : e.g. "GET / HTTP/1.0\r\nHost: www.example.com\r\n\r\n"
 *
 *  respbuf  : response buffer address (allow NULL)
 *
 *  resbuf_len
 *           : resopnse buffer size (allow 0)
 *
 *  (optional)
 *
 *  use_ssl  : true - use HTTPS, false - use HTTP
 *           : default = false
 *
 *  port     : remote port number
 *           : default = (use_ssl) ? 443 : 80
 *
 *  ssl_verify
 *           : true  - use espconn_secure_ca_enable()
 *           : false - use espconn_secure_ca_disable()
 *           : default = false
 *
 *  flash_sector_of_cert
 *           : flash sector of esp_ca_cert.bin
 *           : default = 0x00  See "ESP8266 SSL User Manual"
 *
 *  See "ESP8266 SSL User Manual" for details.
 *
 * [Return Value]
 *
 *   0 : SUCCESS
 *  -1 : Not Connected to AP
 *  -2 : timeout (some error occurred)
 *
 * [Example]
 *
 *  // HTTP request, port = 80, discard the response
 *  sts = doRequest("www.example.com",
 *                  "GET / HTTP/1.0\r\nHost: www.example.com\r\n\r\n",
 *                  NULL, 0);
 *
 *  // HTTP request, port = 80, get the response
 *  sts = doRequest("www.example.com",
 *                  "GET / HTTP/1.0\r\nHost: www.example.com\r\n\r\n",
 *                  buf, sizeof(buf));
 *
 *  // HTTP request, potrt = 8080, get the response
 *  sts = doRequest("www.example.com",
 *                  "GET / HTTP/1.0\r\nHost: www.example.com\r\n\r\n",
 *                  buf, sizeof(buf), false, 8080);
 *
 *  // HTTPS request, port = 443, get the response
 *  // use espconn_secure_ca_disable()
 *  sts = doRequest("www.example.com",
 *                  "GET / HTTP/1.0\r\nHost: www.example.com\r\n\r\n",
 *                  buf, sizeof(buf), true);
 *
 *  // HTTPS request, port = 8443, get the response
 *  // use espconn_secure_ca_disable()
 *  sts = doRequest("www.example.com",
 *                  "GET / HTTP/1.0\r\nHost: www.example.com\r\n\r\n",
 *                  buf, sizeof(buf), true, 8443);
 *
 *  // HTTPS request, port = 443, get the response,
 *  // use espconn_secure_ca_enable(), esp_ca_cert.bin is at 0x65000
 *  sts = doRequest("www.example.com",
 *                  "GET / HTTP/1.0\r\nHost: www.example.com\r\n\r\n",
 *                  buf, sizeof(buf), true, 443, true, 0x65);
 */

#endif // EspHttpClient_H
