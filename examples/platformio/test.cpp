#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "Not supported"
#endif  // ESP8266

#include "MQTTCon.h"

// passed variables
static const uint32_t baud = MY_BAUD;
static const char *ssid = MY_SSID;
static const char *passwd = MY_PASSWD;
static const char *mqttHost = MY_MQTT_HOST;
static const uint16_t mqttPort = MY_MQTT_PORT;
static const char *caCertFile = MY_CA_CERT_FILE;
static const char *certFile = MY_CERT_FILE;
static const char *keyFile = MY_KEY_FILE;
static const char *ntpServer = MY_NTP_SERVER;
static const char *timeZone = MY_TIME_ZONE;

// global constants
static const uint16_t delayMedium = 250;
static const uint16_t delayLong = 1000;
static const uint32_t interval = 30000;

// major classes
MQTTCon mqttCon;

// global variables
uint32_t currentMillis;
uint32_t previousMillis = 0;
uint16_t interval_count = 0;
bool ledStatus = false;

void wifi_setup(void) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passwd);
    Serial.println("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Waiting to connect to WiFi");
        delay(delayMedium);
    }
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
}

#if defined(ESP8266)
// have ntp start within 5 seconds of network
uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000() {
    randomSeed(A0);
    return random(5000);
}
#endif  // ESP8266

void printTime(void) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    Serial.print("Local time: ");
    Serial.print(asctime(&timeinfo));
}

void waitForTime(void) {
    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("");
    printTime();
}

void setup() {
    Serial.begin(baud);
    delay(delayLong * 5);
    Serial.println("Started");
    pinMode(LED_BUILTIN, OUTPUT);
    wifi_setup();
#if defined(ESP8266)
    configTime(timeZone, ntpServer);
#elif defined(ESP32)
    configTime(0, 0, ntpServer);
    setenv("TZ", timeZone, 1);
    tzset();
#endif  // ESP32
    waitForTime();
    mqttCon.setup(mqttHost, mqttPort, caCertFile, certFile, keyFile);
    Serial.println("MQTT setup");
    if (mqttCon.check()) {
        Serial.println("MQTT Connected");
    } else {
        Serial.println("MQTT Connect Failed");
    }
}

void blink(void) {
    Serial.println("Blink");
    ledStatus = !ledStatus;
    digitalWrite(LED_BUILTIN, ledStatus);
}

void loop() {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        printTime();
        if (mqttCon.check()) {
            Serial.println("MQTT Connected");
        } else {
            Serial.println("MQTT Connect Failed");
        }
        blink();
        switch (WiFi.status()) {
            case WL_NO_SSID_AVAIL:
                Serial.println("Configured SSID cannot be reached");
                break;
            case WL_CONNECTED:
                Serial.print("Connected. RSSI: ");
                Serial.println(WiFi.RSSI());
                break;
            case WL_CONNECT_FAILED:
                Serial.println("Connection failed");
                break;
            default:
                break;
        }
        previousMillis = currentMillis;
    }
}
