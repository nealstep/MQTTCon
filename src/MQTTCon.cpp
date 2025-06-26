#include "MQTTCon.h"

// TODO Keep Alive, Will

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "Not supported"
#endif  // ESP8266 or ESP32

#include <LittleFS.h>
#include <time.h>

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

MQTTCon::MQTTCon(void) {
    error = NONE;
    mqtt = &mqttClient;
}

bool MQTTCon::setup(const char *mqttHost, uint16_t mqttPort, const char *caFile,
                    const char *certFile, const char *keyFile,
                    const char *healthTopic) {
    strncpy(mqttID, WiFi.getHostname(), MQTT_ID_SIZE - 2);
    mqttID[MQTT_ID_SIZE - 1] = '\0';
    mqttClient.setServer(mqttHost, mqttPort);
    LittleFS.begin();
#if defined(ESP8266)
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
#elif defined(ESP32)
    caCert = new char[MQTTCON_BUFFER_SIZE];
    getFile(caFile, caCert, MQTTCON_BUFFER_SIZE);
    clientCert = new char[MQTTCON_BUFFER_SIZE];
    getFile(certFile, clientCert, MQTTCON_BUFFER_SIZE);
    clientKey = new char[MQTTCON_BUFFER_SIZE];
    getFile(keyFile, clientKey, MQTTCON_BUFFER_SIZE);
#endif  // ESP
    LittleFS.end();

#if defined(ESP8266)
    espClient.setSSLVersion(BR_TLS12, BR_TLS12);
    espClient.setTrustAnchors(caCert);
    espClient.setClientRSACert(clientCert, clientKey);
#elif defined(ESP32)
    // does not seem to be away to force TLS1.2
    espClient.setCACert(caCert);
    espClient.setCertificate(clientCert);
    espClient.setPrivateKey(clientKey);
#endif  // ESP
    if (snprintf(health, TOPIC_SIZE, "%s/%s", healthTopic, mqttID) < 1) {
        strncpy(health, healthTopic, TOPIC_SIZE - 2);
        health[TOPIC_SIZE - 1] = '\0';
    }
    return true;
}

bool MQTTCon::getFile(const char *file, char *buffer, size_t len) {
    size_t bytes;

    File fp = LittleFS.open(file, "r");
    if (fp.size() >= len) {
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

#if defined(ESP8266)
X509List *MQTTCon::getCert(const char *file) {
    char *buffer;
    X509List *list;

    buffer = new char[MQTTCON_BUFFER_SIZE];
    if (!buffer) {
        error = NEW_FAILED;
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
    if (!buffer) {
        error = NEW_FAILED;
        return nullptr;
    }
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
#endif  // ESP8266

void MQTTCon::getTimeStamp(char *buffer, size_t len) {
    time_t now = time(nullptr);
    struct tm timeInfo;
    gmtime_r(&now, &timeInfo);
    if (!strftime(buffer, len, "%Y-%m-%d@%H:%M:%S", &timeInfo)) {
        buffer[0] = '\0';
    }
}

bool MQTTCon::check() {
    uint8_t count = 0;

    while (!mqttClient.connected()) {
        if (mqttClient.connect(mqttID, health, 0, false, "died")) {
            break;
        } else {
            if (++count > MQTT_ERROR_MAX) {
                error = CONNECT_FAILED;
                return false;
            }
        }
    }
    getTimeStamp(timeStr, TIME_STR_SIZE);
    snprintf(aliveMsg, ALIVE_MSG_SIZE, "alive@%s", timeStr);
    mqttClient.publish(health, aliveMsg);
    error = NONE;
    return true;
}