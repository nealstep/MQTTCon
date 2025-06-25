#include "MQTTCon.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "Not supported"
#endif  // ESP8266 or ESP32

#include <LittleFS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

MQTTCon::MQTTCon(void) {
    error = NONE;
    strncpy(mqttID, WiFi.hostname().c_str(), MQTT_ID_SIZE - 2);
    mqttID[MQTT_ID_SIZE - 1] = '\0';
}

bool MQTTCon::setup(const char *mqttHost, uint16_t mqttPort, const char *caFile,
                 const char *certFile, const char *keyFile) {
    timeClient.begin();
    LittleFS.begin();
    if (!getCert(caFile, caCert)) {
        return false;
    }
    if (!getCert(certFile, clientCert)) {
        return false;
    }
    if (!getKey(keyFile, clientKey)) {
        return false;
    }
    LittleFS.end();
    timeClient.update();
    espClient.setX509Time(timeClient.getEpochTime());
    espClient.setSSLVersion(BR_TLS12, BR_TLS12);
    espClient.setTrustAnchors(caCert);
    espClient.setClientRSACert(clientCert, clientKey);
    return true;
}

bool MQTTCon::getFile(const char *file, char buffer[MQTTCON_BUFFER_SIZE]) {
    size_t bytes;

    File fp = LittleFS.open(file, "r");
    if (fp.size() > MQTTCON_BUFFER_SIZE) {
        error = TOO_BIG;
        return false;
    }
    bytes = fp.readBytes(buffer, fp.size());
    if (bytes != fp.size()) {
        error = SHORT_READ;
        return false;
    }
    error = NONE;
    return true;
}

bool MQTTCon::getCert(const char *file, X509List *list) {
    char buffer[MQTTCON_BUFFER_SIZE];

    if (getFile(file, buffer)) {
        list = new X509List(buffer);
        if (!list) {
            error = NEW_FAILED;
            return false;
        }
    } else {
        return false;
    }
    error = NONE;
    return true;
}

bool MQTTCon::getKey(const char *file, PrivateKey *key) {
    char buffer[MQTTCON_BUFFER_SIZE];

    if (getFile(file, buffer)) {
        key = new PrivateKey(buffer);
        if (!key) {
            error = NEW_FAILED;
            return false;
        }
    } else {
        return false;
    }
    error = NONE;
    return true;
}

bool MQTTCon::check() {
    uint8_t count = 0;
    
    timeClient.update();
    while (!mqttClient.connected()) {
        if (mqttClient.connect(mqttID)) {
            error = NONE;
            break;
        } else {
            if (++count > MQTT_ERROR_MAX) {
                error = CONNECT_FAILED;
                return false;
            }
        }
    }
    return true;
}