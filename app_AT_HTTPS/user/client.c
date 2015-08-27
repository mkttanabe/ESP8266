//
// client.c
//
// 2015 KLab Inc.
//
// based on "TCP SSL client" sample code developed by ESPRESSIF SYSTEMS
//
// http://bbs.espressif.com/viewtopic.php?f=21&t=389
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
 * this software and associated documentation files (the OSoftwareP), 
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED OAS ISP, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
*******************************************************************************/
#include "c_types.h"
#include "ip_addr.h"
#include "os_type.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "mem.h"

#define SSID "YOUR SSID"
#define PASSWORD "YOUR PASSWORD"

#define PORT_HTTP    80
#define PORT_HTTPS 443

#define SSL_BUFFER_SIZE  8192

const char* REQUEST_HEADER = "GET / HTTP/1.0\r\nHost: %s\r\n\r\n";

uint8 _hostname[128];
int _use_ssl, _ssl_verify;

#define packet_size    (2 * 1024)

unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;

LOCAL os_timer_t test_timer;
LOCAL struct espconn user_tcp_conn;
LOCAL struct _esp_tcp user_tcp;
ip_addr_t tcp_server_ip;

/******************************************************************************
 * FunctionName : user_tcp_recv_cb
 * Description  : receive callback.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
    //received some data from tcp connection
    os_printf("tcp recv !!! [%s]\r\n", pusrdata);
}
/******************************************************************************
 * FunctionName : user_tcp_sent_cb
 * Description  : data sent callback.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_tcp_sent_cb(void *arg)
{
    //data sent successfully
    os_printf("tcp sent succeed !!! \r\n");
}
/******************************************************************************
 * FunctionName : user_tcp_discon_cb
 * Description  : disconnect callback.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_tcp_discon_cb(void *arg)
{
    //tcp disconnect successfully
    os_printf("tcp disconnect succeed !!! \r\n");
}
/******************************************************************************
 * FunctionName : user_esp_platform_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters    : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_sent_data(struct espconn *pespconn)
{
    char *pbuf = (char *)os_zalloc(packet_size);
    os_sprintf(pbuf, REQUEST_HEADER, _hostname);
    if (_use_ssl) {
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
    } else {
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));
    }
    os_free(pbuf);
}

/******************************************************************************
 * FunctionName : user_tcp_connect_cb
 * Description  : A new incoming tcp connection has been connected.
 * Parameters    : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_tcp_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;
    os_printf("connect succeed !!! \r\n");
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
LOCAL void ICACHE_FLASH_ATTR
user_tcp_recon_cb(void *arg, sint8 err)
{
    os_printf("reconnect callback, error code %d !!! \r\n",err);
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
LOCAL void ICACHE_FLASH_ATTR
user_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;

    if (ipaddr == NULL) {
        os_printf("user_dns_found NULL \r\n");
        return;
    }

    //dns got ip
    os_printf("user_dns_found %d.%d.%d.%d \r\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

    if (tcp_server_ip.addr == 0 && ipaddr->addr != 0)  {
        os_printf("connecting to [%s] use_ssl=%d\r\n", _hostname, _use_ssl);
        // dns succeed, create tcp connection
        os_timer_disarm(&test_timer);
        tcp_server_ip.addr = ipaddr->addr;
        os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4); // remote ip of tcp server which get by dns

        system_print_meminfo();
        os_printf("system_get_free_heap_size=%ld\r\n", system_get_free_heap_size());

        if (_use_ssl) {
            user_tcp_conn.proto.tcp->remote_port = PORT_HTTPS;
        } else {
            user_tcp_conn.proto.tcp->remote_port = PORT_HTTP;
        }
        pespconn->proto.tcp->local_port = espconn_port(); //local port of ESP8266

        espconn_regist_connectcb(pespconn, user_tcp_connect_cb); // register connect callback
        espconn_regist_reconcb(pespconn, user_tcp_recon_cb); // register reconnect callback as error handler
        if (_use_ssl) {
            if (_ssl_verify) {
                os_printf("espconn_secure_ca_enable=%d\r\n", espconn_secure_ca_enable(0x01, 0x65)); // 0x01 = client
            } else {
                os_printf("espconn_secure_ca_disable=%d\r\n", espconn_secure_ca_disable(0x01));
            }
            os_printf("espconn_secure_set_size=%d\r\n", SSL_BUFFER_SIZE);
            espconn_secure_set_size(ESPCONN_CLIENT, SSL_BUFFER_SIZE); // set SSL buffer size, if your SSL packet larger than 2048 bytes
            os_printf("start espconn_secure_connect\r\n");
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
LOCAL void ICACHE_FLASH_ATTR
user_dns_check_cb(void *arg)
{
    struct espconn *pespconn = arg;
    espconn_gethostbyname(pespconn, _hostname, &tcp_server_ip, user_dns_found); // recall DNS function
    os_timer_arm(&test_timer, 1000, 0);
}

/******************************************************************************
 * FunctionName : user_check_ip
 * Description  : check whether get ip addr or not
 * Parameters    : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_check_ip(void)
{
    struct ip_info ipconfig;

    //disarm timer first
    os_timer_disarm(&test_timer);

    //get ip info of ESP8266 station
    wifi_get_ip_info(STATION_IF, &ipconfig);

    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
        //os_printf("got ip !!! \r\n");

        // Connect to tcp server
        user_tcp_conn.proto.tcp = &user_tcp;
        user_tcp_conn.type = ESPCONN_TCP;
        user_tcp_conn.state = ESPCONN_NONE;

        tcp_server_ip.addr = 0;
        espconn_gethostbyname(&user_tcp_conn, _hostname, &tcp_server_ip, user_dns_found); // DNS function

        os_timer_setfn(&test_timer, (os_timer_func_t *)user_dns_check_cb, user_tcp_conn);
        os_timer_arm(&test_timer, 1000, 0);
    }
    else {
        if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
            wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
            wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) {
            os_printf("connect fail !!! \r\n");
        } 
        else  {
            //re-arm timer to check ip
            os_timer_setfn(&test_timer, (os_timer_func_t *)user_check_ip, NULL);
            os_timer_arm(&test_timer, 100, 0);
        }
    }
}

/******************************************************************************
 * FunctionName : user_set_station_config
 * Description  : set the router info which ESP8266 station will connect to 
 * Parameters    : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_set_station_config(void)
{
    // Wifi configuration 
    char ssid[32] = SSID; 
    char password[64] = PASSWORD; 
    struct station_config stationConf; 

    //need not mac address
    stationConf.bssid_set = 0; 
    
    //Set ap settings 
    os_memcpy(&stationConf.ssid, ssid, 32); 
    os_memcpy(&stationConf.password, password, 64); 
    wifi_station_set_config(&stationConf); 

    wifi_station_disconnect();
    wifi_station_connect();

    //set a timer to check whether got ip from router succeed or not.
    os_timer_disarm(&test_timer);
    os_timer_setfn(&test_timer, (os_timer_func_t *)user_check_ip, NULL);
    os_timer_arm(&test_timer, 100, 0);
}

void doit(uint8 *hostname, int use_ssl, int ssl_verify)
{
    _use_ssl = use_ssl;
    _ssl_verify = ssl_verify;
    os_strcpy(_hostname, hostname);

    if (wifi_station_get_connect_status() != STATION_GOT_IP) {
        os_printf("SDK version:%s\n", system_get_sdk_version());
        wifi_set_opmode(STATION_MODE);
        user_set_station_config();
    } else {
        user_check_ip();
    }
}

