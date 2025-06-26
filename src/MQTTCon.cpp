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
#include <PubSubClient.h>
#include <time.h>

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

MQTTCon::MQTTCon(void) {
    error = NONE;
    strncpy(mqttID, WiFi.hostname().c_str(), MQTT_ID_SIZE - 2);
    mqttID[MQTT_ID_SIZE - 1] = '\0';
}

bool MQTTCon::setup(const char *mqttHost, uint16_t mqttPort, const char *caFile,
                    const char *certFile, const char *keyFile) {
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

bool MQTTCon::check() {
    uint8_t count = 0;

    while (!mqttClient.connected()) {
        if (mqttClient.connect(mqttID)) {
            break;
        } else {
            if (++count > MQTT_ERROR_MAX) {
                error = CONNECT_FAILED;
                return false;
            }
        }
    }
    mqttClient.publish("test", "check");
    error = NONE;
    return true;
}