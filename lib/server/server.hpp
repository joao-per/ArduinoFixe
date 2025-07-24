#ifndef __SERVER__HPP
#define __SERVER__HPP

#include <string>
#include <HardwareSerial.h>

class server {
public:
    server(std::string ssid, std::string password, std::string ip, int port) :
        ssid(ssid), password(password), ip(ip), port(port) {}

    const char* get_ssid();
    const char* get_password();
    const char* get_ip();
    int get_port();

private:
    std::string ssid;
    std::string password;
    std::string ip;
    int port;
};

#endif
