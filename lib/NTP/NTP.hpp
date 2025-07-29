#ifndef NTP_HPP
#define NTP_HPP

#include <Arduino.h>
#include <WiFiEspAT.h>
#include "set_rtc.hpp"

// Servidor NTP em Portugal
#define NTP_SERVER "pt.pool.ntp.org"
#define NTP_PORT 123
#define NTP_PACKET_SIZE 48

class NTPClient {
private:
    WiFiUDP udp;
    byte packetBuffer[NTP_PACKET_SIZE];
    
    void sendNTPpacket(const char* address) {
        memset(packetBuffer, 0, NTP_PACKET_SIZE);
        packetBuffer[0] = 0b11100011;   // LI, Version, Mode
        packetBuffer[1] = 0;     // Stratum
        packetBuffer[2] = 6;     // Polling Interval
        packetBuffer[3] = 0xEC;  // Peer Clock Precision
        // 8 bytes zero for Root Delay & Root Dispersion
        packetBuffer[12]  = 49;
        packetBuffer[13]  = 0x4E;
        packetBuffer[14]  = 49;
        packetBuffer[15]  = 52;
        
        udp.beginPacket(address, NTP_PORT);
        udp.write(packetBuffer, NTP_PACKET_SIZE);
        udp.endPacket();
    }
    
public:
    bool updateTime() {
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }
        
        udp.begin(8888);
        sendNTPpacket(NTP_SERVER);
        
        delay(1000);
        
        if (udp.parsePacket()) {
            udp.read(packetBuffer, NTP_PACKET_SIZE);
            
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            
            const unsigned long seventyYears = 2208988800UL;
            unsigned long epoch = secsSince1900 - seventyYears;
            
            // Adicionar timezone Portugal (UTC+1 no verão, UTC no inverno)
            epoch += 3600; // UTC+1
            
            // Converter para data/hora
            int year = 1970;
            unsigned long days = epoch / 86400L;
            
            while (days > 365) {
                if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
                    if (days > 366) {
                        days -= 366;
                        year++;
                    } else break;
                } else {
                    days -= 365;
                    year++;
                }
            }
            
            int month = 1;
            int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
            if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
                daysInMonth[1] = 29;
            }
            
            for (int i = 0; i < 12 && days > daysInMonth[i]; i++) {
                days -= daysInMonth[i];
                month++;
            }
            
            int day = days + 1;
            int hour = (epoch % 86400L) / 3600;
            int minute = (epoch % 3600) / 60;
            int second = epoch % 60;
            
            set_rtc_time(day, month, year, hour, minute, second);
            
            udp.stop();
            return true;
        }
        
        udp.stop();
        return false;
    }
};

#endif // NTP_HPP