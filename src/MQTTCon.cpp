#include "MQTTCon.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "Not supported"
#endif  // ESP8266 or ESP32

#include <LittleFS.h>
#include <PubSubClient.h>
#include <time.h>

#define MY_NTP_SERVER "ca.pool.ntp.org"
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0"

#include "certs.h"

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000() {
    randomSeed(A0);
    return random(5000);
}

MQTTCon::MQTTCon(void) {
    error = NONE;
    strncpy(mqttID, WiFi.hostname().c_str(), MQTT_ID_SIZE - 2);
    mqttID[MQTT_ID_SIZE - 1] = '\0';
    configTime(MY_TZ, MY_NTP_SERVER);
}

bool MQTTCon::setup(const char *mqttHost, uint16_t mqttPort, const char *caFile,
                    const char *certFile, const char *keyFile) {
    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));

    mqttClient.setServer(mqttHost, mqttPort);
    LittleFS.begin();
    caCert = getCert(caFile);
    if (!caCert) {
        return false;
    }
    clientCert = getCert(certFile);
    if (!clientCert) {
        return false;
    }
    clientKey = getKey(keyFile);
    if (!clientKey) {
        return false;
    }
    LittleFS.end();

    espClient.setSSLVersion(BR_TLS12, BR_TLS12);
    espClient.setTrustAnchors(caCert);
    espClient.setClientRSACert(clientCert, clientKey);
    espClient.connect(mqttHost, mqttPort);
    char err[80];
    espClient.getLastSSLError(err, 80);
    err[80 - 1] = '\0';
    Serial.println(err);
    return true;
}

bool MQTTCon::getFile(const char *file, char *buffer, size_t len) {
    size_t bytes;

    File fp = LittleFS.open(file, "r");
    if (fp.size() > len) {
        error = TOO_BIG;
        return false;
    }
    bytes = fp.readBytes(buffer, fp.size());
    if (bytes != fp.size()) {
        error = SHORT_READ;
        return false;
    }
    buffer[bytes] = '\0';
    Serial.print(file);
    Serial.print(" bytes: ");
    Serial.println(bytes);
    Serial.println(buffer);
    error = NONE;
    return true;
}

X509List *MQTTCon::getCert(const char *file) {
    char *buffer;
    X509List *list;

    buffer = new char[MQTTCON_BUFFER_SIZE];
    if (!buffer) {
        error = BUFFER_NEW_FAILED;
        return nullptr;
    }
    if (getFile(file, buffer, MQTTCON_BUFFER_SIZE)) {
        list = new X509List(buffer);
        if (!list) {
            error = NEW_FAILED;
            return nullptr;
        }
    } else {
        return nullptr;
    }
    error = NONE;
    return list;
}

PrivateKey *MQTTCon::getKey(const char *file) {
    char *buffer;
    PrivateKey *key;

    buffer = new char[MQTTCON_BUFFER_SIZE];
    if (getFile(file, buffer, MQTTCON_BUFFER_SIZE)) {
        key = new PrivateKey(buffer);
        if (!key) {
            error = NEW_FAILED;
            return nullptr;
        }
    } else {
        return nullptr;
    }
    error = NONE;
    return key;
}

void showTime() {
    time_t now;
    tm tm;

    time(&now);              // read the current time
    localtime_r(&now, &tm);  // update the structure tm with the current time
    Serial.print(tm.tm_year + 1900);  // years since 1900
    Serial.print("-");
    Serial.print(tm.tm_mon + 1);  // January = 0 (!)
    Serial.print("-");
    Serial.print(tm.tm_mday);  // day of month
    Serial.print("@");
    Serial.print(tm.tm_hour);  // hours since midnight  0-23
    Serial.print(":");
    Serial.print(tm.tm_min);  // minutes after the hour  0-59
    Serial.print(":");
    Serial.print(tm.tm_sec);  // seconds after the minute  0-61*
    if (tm.tm_isdst == 1)     // Daylight Saving Time flag
        Serial.print(" DST");
    else
        Serial.print(" ST");
    Serial.println();
}

bool MQTTCon::check() {
    uint8_t count = 0;

    showTime();
    while (!mqttClient.connected()) {
        if (mqttClient.connect(mqttID)) {
            error = NONE;
            break;
        } else {
            Serial.println("fail");
            if (++count > MQTT_ERROR_MAX) {
                Serial.println("MQTT connect failed");
                char err[80];
                espClient.getLastSSLError(err, 80);
                err[80 - 1] = '\0';
                Serial.println(err);
                error = CONNECT_FAILED;
                return false;
            }
        }
    }
    Serial.println("MQTT connect");
    mqttClient.publish("test", "connected");
    return true;
}