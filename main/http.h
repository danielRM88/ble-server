#ifndef HTTP_H_
 #define HTTP_H_

#define WEB_SERVER "172.20.10.3"
#define WEB_PORT "3000"
#define WEB_URL "/register"

 void send_req(const char* req);
 void init_socket();

 #endif
