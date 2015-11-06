#ifndef OneButtonMailer_IFTTT_H
#define OneButtonMailer_IFTTT_H

// secret: Wi-Fi AP
#define MYSSID   "your ssid"
#define PASS     "your pass"

// IFTTT Maker channel
#define HOSTNAME          "maker.ifttt.com"
#define MAKER_CHANNEL_KEY "dSIwDguVEYfZ1SgiISL***********************"
#define EVENTNAME         "buttonPushed"
#define URI               "/trigger/" EVENTNAME "/with/key/" MAKER_CHANNEL_KEY

#define HEADER \
  "GET " URI " HTTP/1.0\r\n" \
  "Host: " HOSTNAME "\r\n\r\n"

#endif // OneButtonMailer_IFTTT_H
 
