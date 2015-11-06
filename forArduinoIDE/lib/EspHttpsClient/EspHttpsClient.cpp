//
// EspHttpsClient.cpp
//
// 2015 KLab Inc.
//
// based on "TCP SSL client" sample code developed by ESPRESSIF SYSTEMS
// http://bbs.espressif.com/viewtopic.php?f=21&t=389
//
// for "Arduino IDE for ESP8266"
// https://github.com/esp8266/Arduino
//
/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only,
 * in which case, it is free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the ・ｽOSoftware・ｽP),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED ・ｽOAS IS・ｽP, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
*******************************************************************************/
#include <ESP8266WiFi.h>

extern "C" {
#include "c_types.h"
#include "ip_addr.h"
#include "os_type.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"
}

#define DBG os_printf
//#define DBG Serial.printf

#define SSL_BUFFER_SIZE  8192
#define TIMEOUT 15000
#define packet_size 2048

static char _hostname[128];
static boolean _use_ssl, _ssl_verify;
static uint16 _flash_sector_of_cert;
static short _port;
static char *p_resbuf;
static int _resbuf_len;
static char *p_request;
static int done = 0;
static unsigned long starttime;

static os_timer_t test_timer;
static struct espconn user_tcp_conn;
static struct _esp_tcp user_tcp;
static ip_addr_t tcp_server_ip;

#if 0
// from examples/driver_lib/include/driver/uart_register.h
#define REG_UART_BASE(i)   (0x60000000 + (i)*0xf00)
#define UART_STATUS(i)     (REG_UART_BASE(i) + 0x1C)
#define UART_TXFIFO_CNT    0x000000FF
#define UART_TXFIFO_CNT_S  16
#define UART_FIFO(i)       (REG_UART_BASE(i) + 0x0)

// from examples/driver_lib/include/driver/uart.h
#define UART0   0
#define UART1   1

// from examples/driver_lib/driver/uart.c
static int
uart_tx_one_char(uint8 uart, uint8 TxChar)
{
  while (true) {
    uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);

    if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
      break;
    }
  }
  WRITE_PERI_REG(UART_FIFO(uart) , TxChar);
  return 0;
}

static void ICACHE_FLASH_ATTR
uart0_write_char(char c)
{
  if (c == '\n') {
    uart_tx_one_char(UART0, '\r');
    uart_tx_one_char(UART0, '\n');
  } else if (c == '\r') {
  } else {
    uart_tx_one_char(UART0, c);
  }
}
#endif

/******************************************************************************
 * FunctionName : user_tcp_recv_cb
 * Description  : receive callback.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
  done = 1;
  if (!p_resbuf || _resbuf_len <= 0 || *p_resbuf != '\0') {
    return;
  }
  int len = length;
  if (len >= _resbuf_len) {
    len = _resbuf_len - 1;
  }
  //DBG("tcp recv [%s]\r\n", pusrdata);
  os_strncpy(p_resbuf, pusrdata, len);
}
/******************************************************************************
 * FunctionName : user_tcp_sent_cb
 * Description  : data sent callback.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_tcp_sent_cb(void *arg)
{
  //data sent successfully
  DBG("tcp sent succeed\r\n");
}
/******************************************************************************
 * FunctionName : user_tcp_discon_cb
 * Description  : disconnect callback.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_tcp_discon_cb(void *arg)
{
  //tcp disconnect successfully
  DBG("tcp disconnect succeed\r\n");
}
/******************************************************************************
 * FunctionName : user_esp_platform_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters    : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_sent_data(struct espconn *pespconn)
{
  if (_use_ssl) {
    espconn_secure_sent(pespconn, (uint8*)p_request, os_strlen(p_request));
  } else {
    espconn_sent(pespconn, (uint8*)p_request, os_strlen(p_request));
  }
}

/******************************************************************************
 * FunctionName : user_tcp_connect_cb
 * Description  : A new incoming tcp connection has been connected.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_tcp_connect_cb(void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;
  DBG("connect succeed\r\n");
  espconn_regist_recvcb(pespconn, user_tcp_recv_cb);
  espconn_regist_sentcb(pespconn, user_tcp_sent_cb);
  espconn_regist_disconcb(pespconn, user_tcp_discon_cb);
  user_sent_data(pespconn);
}

/******************************************************************************
 * FunctionName : user_tcp_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_tcp_recon_cb(void *arg, sint8 err)
{
  DBG("reconnect callback, error code %d\r\n", err);
}

/******************************************************************************
 * FunctionName : user_dns_found
 * Description  : dns found callback
 * Parameters    : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
  struct espconn *pespconn = (struct espconn *)arg;

  if (ipaddr == NULL) {
    DBG("user_dns_found NULL \r\n");
    return;
  }

  //dns got ip
  DBG("user_dns_found %d.%d.%d.%d \r\n",
      *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
      *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));


  if (tcp_server_ip.addr == 0 && ipaddr->addr != 0)  {
    DBG("connecting to [%s] use_ssl=%d\r\n", _hostname, _use_ssl);
    // dns succeed, create tcp connection
    os_timer_disarm(&test_timer);
    tcp_server_ip.addr = ipaddr->addr;
    os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4); // remote ip of tcp server which get by dns

    system_print_meminfo();
    DBG("system_get_free_heap_size=%ld\r\n", system_get_free_heap_size());
    user_tcp_conn.proto.tcp->remote_port = _port;
    pespconn->proto.tcp->local_port = espconn_port(); //local port of ESP8266

    espconn_regist_connectcb(pespconn, user_tcp_connect_cb); // register connect callback
    espconn_regist_reconcb(pespconn, user_tcp_recon_cb); // register reconnect callback as error handler
    if (_use_ssl) {
      if (_ssl_verify && _flash_sector_of_cert != 0x00) {
        DBG("espconn_secure_ca_enable=%d\r\n", espconn_secure_ca_enable(0x01, _flash_sector_of_cert)); // 0x01 = client
      } else {
        DBG("espconn_secure_ca_disable=%d\r\n", espconn_secure_ca_disable(0x01));
      }
      DBG("espconn_secure_set_size=%d\r\n", SSL_BUFFER_SIZE);
      espconn_secure_set_size(ESPCONN_CLIENT, SSL_BUFFER_SIZE); // set SSL buffer size, if your SSL packet larger than 2048 bytes
      DBG("start espconn_secure_connect\r\n");
      espconn_secure_connect(pespconn); // tcp SSL connect
    } else {
      espconn_connect(pespconn);
    }
  }
}

/******************************************************************************
 * FunctionName : user_esp_platform_dns_check_cb
 * Description  : 1s time callback to check dns found
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_dns_check_cb(void *arg)
{
  struct espconn *pespconn = (struct espconn*)arg;
  espconn_gethostbyname(pespconn, (char*)_hostname, &tcp_server_ip, user_dns_found); // recall DNS function
  os_timer_arm(&test_timer, 1000, 0);
}

/******************************************************************************
 * FunctionName : user_check_ip
 * Description  : check whether get ip addr or not
 * Parameters    : none
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR
user_check_ip(void)
{
  struct ip_info ipconfig;

  //disarm timer first
  os_timer_disarm(&test_timer);

  //get ip info of ESP8266 station
  wifi_get_ip_info(STATION_IF, &ipconfig);

  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    //DBG("got ip\r\n");

    // Connect to tcp server
    user_tcp_conn.proto.tcp = &user_tcp;
    user_tcp_conn.type = ESPCONN_TCP;
    user_tcp_conn.state = ESPCONN_NONE;

    tcp_server_ip.addr = 0;
    espconn_gethostbyname(&user_tcp_conn, (char*)_hostname, &tcp_server_ip, user_dns_found); // DNS function

    os_timer_setfn(&test_timer, (os_timer_func_t *)user_dns_check_cb, &user_tcp_conn);
    os_timer_arm(&test_timer, 1000, 0);
  }
  else {
    if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
         wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
         wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) {
      DBG("connect fail !!! \r\n");
    }
    else  {
      //re-arm timer to check ip
      os_timer_setfn(&test_timer, (os_timer_func_t *)user_check_ip, NULL);
      os_timer_arm(&test_timer, 100, 0);
    }
  }
}

// public
int doRequest(const char *hostname,
              const char *request,
              char *resbuf,
              int resbuf_len,
              boolean use_ssl,
              short port,
              boolean ssl_verify,
              uint16 flash_sector_of_cert)
{
  starttime = millis();
#if 0
  os_install_putc1((void *)uart0_write_char);
#endif
  os_strcpy(_hostname, hostname);
  p_request = (char*)request;
  p_resbuf = resbuf;
  _resbuf_len = resbuf_len;
  os_memset(p_resbuf, '\0', _resbuf_len);
  _use_ssl = use_ssl;
  _ssl_verify = ssl_verify;
  _flash_sector_of_cert = flash_sector_of_cert;
  _port = port;
  if (_port == 0) {
    _port = (_use_ssl) ? 443 : 80;
  }

  if (wifi_station_get_connect_status() != STATION_GOT_IP) {
    return -1;
  }
  user_check_ip();
  while (!done) {
    if (millis() - starttime > TIMEOUT) {
      return -2;
    }
    delay(100);
  }
  return (done) ? 0 : 1;
}
