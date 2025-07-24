#include "server.hpp"

const char *server::get_ssid() 
{
    return ssid.c_str();
}

const char *server::get_password()
{ 
    return password.c_str(); 
}

const char *server::get_ip() 
{
    return ip.c_str(); 
}

int server::get_port() 
{ 
    return port;
}