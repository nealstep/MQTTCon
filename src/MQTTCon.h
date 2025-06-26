#ifndef MQTTCON_H
#define MQTTCON_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

#define MQTTCON_BUFFER_SIZE 2048
#define MQTT_ID_SIZE 16
#define MQTT_ERROR_MAX 5

class MQTTCon {
   public:
    enum errors { NONE, TOO_BIG, SHORT_READ, NEW_FAILED, CONNECT_FAILED };
    errors error;
    MQTTCon(void);
    bool setup(const char *mqttHost, uint16_t mqttPort, const char *caFile,
               const char *certFile, const char *keyFile);
    bool check();

   private:
    X509List *caCert, *clientCert;
    PrivateKey *clientKey;
    char mqttID[MQTT_ID_SIZE];

    bool getFile(const char *file, char *buffer, size_t len);
    X509List *getCert(const char *file);
    PrivateKey *getKey(const char *file);
};

#endif  // MQTTCON_H
