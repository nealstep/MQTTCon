#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#error "Not supported"
#endif  // ESP8266

#include "MQTTCon.h"

static const char *ssid = MY_SSID;
static const char *passwd = MY_PASSWD;
static const char *udp_logger_ip = MY_UDP_LOGGER_IP;
ADD a lot here
static uint16_t delay_medium = 250;

UDPLogger udpLogger;

uint32_t currentMillis;
uint32_t previousMillis = 0;
uint32_t interval = 30000;
uint16_t interval_count = 0;
bool ledStatus = false;

void wifi_setup(void) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passwd);
    udpLogger.log(udpLogger.INFO, "Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        udpLogger.log(udpLogger.DEBUG, "Waiting to connect");
        delay(delay_medium);
    }
    udpLogger.log(udpLogger.INFO, WiFi.localIP().toString());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
}

void setup() {
    udpLogger.initSerial();
    udpLogger.initUDP(udp_logger_ip);
    udpLogger.setLevel(udpLogger.DEBUG);
    udpLogger.log(udpLogger.INFO, "Strarting");
    pinMode(LED_BUILTIN, OUTPUT);
    wifi_setup();
}

void blink(void) {
    udpLogger.log(udpLogger.DEBUG, "Blink");
    ledStatus = !ledStatus;
    digitalWrite(LED_BUILTIN, ledStatus);
}

void loop() {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        blink();
        switch (WiFi.status()) {
            case WL_NO_SSID_AVAIL:
                udpLogger.log(udpLogger.ERROR,
                              "Configured SSID cannot be reached");
                break;
            case WL_CONNECTED:
                udpLogger.log(udpLogger.INFO,
                              "Connection successfully established");
                break;
            case WL_CONNECT_FAILED:
                udpLogger.log(udpLogger.WARNING, "Connection failed");
                break;
            default:
                break;
        }
        udpLogger.value("RSSI", WiFi.RSSI());
        interval_count++;
        if (interval_count == 10) {
            udpLogger.setLevel(udpLogger.INFO);
        }
        previousMillis = currentMillis;
    }
}
