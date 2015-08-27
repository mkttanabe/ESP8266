// user_main.c
//
// 2015 KLab Inc.
//
// based on "esp_iot_sdk_v1.3.0/example/at/user_main.c"
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
 * this software and associated documentation files (the ＾Software￣), 
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED ＾AS IS￣, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
*******************************************************************************/
// ATコマンド拡張試作
//
// AT+GETTEST="hostname" で当該ホストへ "GET /" リクエストを送る
//
// AT+GETTEST  --- 引数がなければ www.example.com:80 へ HTTP リクエスト
//
// AT+GETTEST="www.example.com",0 --- 2nd arg が 0 なら 80番ポートへ HTTP リクエスト
// AT+GETTEST="www.example.com",1 --- 2nd arg が 1 なら 443番ポートへ HTTPS リクエスト
//
// AT+GETTEST="www.example.com",1,0 --- 3rd arg が 0 ならサーバ証明書検証なしで 443番ポートへ HTTPS リクエスト
// AT+GETTEST="www.example.com",1,1 --- 3rd arg が 1 ならサーバ証明書検証ありで 443番ポートへ HTTPS リクエスト
//

#include "osapi.h"
#include "at_custom.h"
#include "user_interface.h"

#define EXAMPLE_COM "www.example.com"

uint8 buffer[64] = {0};

extern void doit(uint8 *hostname, int use_ssl, int ssl_verify);

#define UART0   0
#define UART1   1

LOCAL void ICACHE_FLASH_ATTR
_uart0_write_char(char c)
{
    if (c == '\n') {
        uart_tx_one_char(UART0, '\r');
        uart_tx_one_char(UART0, '\n');
    } else if (c == '\r') {
    } else {
        uart_tx_one_char(UART0, c);
    }
}

void ICACHE_FLASH_ATTR
at_exeCmdGetTest(uint8_t id)
{
    os_strcpy(buffer, EXAMPLE_COM);
    doit(buffer, 0, 0);
}

void ICACHE_FLASH_ATTR
at_setupCmdGetTest(uint8_t id, char *pPara)
{
    int result, err;
    int use_ssl, ssl_verify = 0;
    pPara++; // skip '='
    at_data_str_copy(buffer, &pPara, 64);

    if (*pPara++ != ',') { // skip ','
        use_ssl = 0;
    } else {
        at_get_next_int_dec(&pPara, &result, &err);
        use_ssl = result;
    }
    if (use_ssl) {
        if (*pPara++ != ',') {
            ssl_verify = 0;
        } else {
            at_get_next_int_dec(&pPara, &result, &err);
            ssl_verify = result;
        }
    }
    doit(buffer, use_ssl, ssl_verify);
    at_response_ok();
}

/*
 http://bbs.espressif.com/viewtopic.php?t=837

 Q: How do I define my own functions in the AT+ commands? How do I pass parameters between them?

 In the example of \esp_iot_sdk\examples\at\user\user_main.c, 
 ways are delivered on how to implement a self-defined AT Command, “ AT+TEST”.
 The structure, at_funcationType, is used to define four types of a command, e.g., “AT+TEST”. 

 "at_testCmd" is a testing command and it’s formatted as AT+TEST=?. In the example of AT, 
 the registered callback is “at_testCmdTest”; the testing demand could be designed as
 the value range of the return parameter. If registered as NULL, there will be no testing command.

 "at_queryCmd" is a query command and it’s formatted as AT+TEST?.In the example of AT, 
 the registered callback is “at_queryCmdTest” ; the query command could be designed as returning
  the current value. If registered as NULL, there will be no query command.

 "at_setupCmd" is a setup command and it’s formatted as AT+TEST=parameter1,parameter2,........
  In the example of AT, the registered callback is "at_setupCmdTest"; the setup command could be
  designed as the value of the parameter; if registered as NULL, there will be no setup command.

 "at_exeCmd" is an execution command and it’s formatted as AT+TEST. In the example of AT, 
 the registered callback is “at_exeCmdTest”; if registered as NULL, there will be no execution command.
*/
at_funcationType at_custom_cmd[] = {
    {"+GETTEST", 8, NULL, NULL, at_setupCmdGetTest, at_exeCmdGetTest},
};

void user_rf_pre_init(void)
{
}

void user_init(void)
{
    char buf[64] = {0};
    at_customLinkMax = 5;
    at_init();
    os_sprintf(buf,"compile time:%s %s",__DATE__,__TIME__);
    at_set_custom_info(buf);
    at_port_print("\r\nready\r\n");
    at_cmd_array_regist(&at_custom_cmd[0], sizeof(at_custom_cmd)/sizeof(at_custom_cmd[0]));
    // os_printf の出力を UART0 に
    os_install_putc1((void *)_uart0_write_char);
}
