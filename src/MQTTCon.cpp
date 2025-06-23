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

WiFiClientSecure espClient;
PubSubClient client(espClient);

MQTTCon::MQTTCon(void) {
    error = NONE;
}

bool MQTTCon::setup(const char *mqttHost, uint16_t mqttPort, const char *caFile,
                 const char *certFile, const char *keyFile) {
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
