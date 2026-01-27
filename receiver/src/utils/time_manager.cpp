#include "time_manager.h"
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <RTC.h>

static const char* ntpServerName = "pool.ntp.org";
static const unsigned int localPort = 2390; // Local port to listen for UDP packets
static const int NTP_PACKET_SIZE = 48;
static byte packetBuffer[NTP_PACKET_SIZE]; 

WiFiUDP Udp;

// Sends an NTP request to the time server at the given address
static void sendNTPpacket(const char* address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

bool syncTimeWithNTP() {
    Serial.println("[TimeManager] Syncing NTP...");
    Udp.begin(localPort);
    
    sendNTPpacket(ntpServerName);
    
    // Wait for response (max 1.5s)
    unsigned long start = millis();
    while (Udp.parsePacket() == 0) {
        if (millis() - start > 1500) {
            Serial.println("[TimeManager] Timeout waiting for NTP response.");
            Udp.stop();
            return false;
        }
        delay(10);
    }
    
    // Read the packet into the buffer
    Udp.read(packetBuffer, NTP_PACKET_SIZE);

    // The timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    
    // Combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;

    // Update RTC
    RTCTime timeToSet(epoch); 
    RTC.setTime(timeToSet);

    Serial.print("[TimeManager] RTC Updated. Unix Time: ");
    Serial.println(epoch);

    Udp.stop();
    return true;
}
