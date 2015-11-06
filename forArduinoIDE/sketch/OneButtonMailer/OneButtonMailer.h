#ifndef OneButtonMailer_H
#define OneButtonMailer_H

// secret: Wi-Fi AP
#define MYSSID   "your ssid"
#define PASS     "your pass"

// secret: SendGrid account
#define API_USER "your id"
#define API_KEY  "your key"

// mail settings
#define FROM     "from mail address"
#define FROMNAME "from name"
#define TO       FROM
#define TONAME   FROMNAME
#define SUBJECT  "***+NOTIFICATION+***"
#define TEXT     "Button+is+pushed."

// SendGrid service
#define HOSTNAME "api.sendgrid.com"
#define URI      "/api/mail.send.json"

// request message
#define HEADER \
  "POST " URI " HTTP/1.0\r\n" \
  "Content-Type: application/x-www-form-urlencoded\r\n" \
  "Host: " HOSTNAME "\r\n" \
  "Content-Length: %d\r\n\r\n"

#define BODY \
  "api_user=" API_USER "&" \
  "api_key="  API_KEY  "&" \
  "to="       TO       "&" \
  "toname="   TONAME   "&" \
  "subject="  SUBJECT  "&" \
  "text="     TEXT     "&" \
  "from="     FROM     "&" \
  "fromname=" FROMNAME

#endif // OneButtonMailer_H
 
