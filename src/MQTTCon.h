#ifndef MQTTCON_H
#define MQTTCON_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

#define MQTTCON_BUFFER_SIZE 2048

class MQTTCon {
   public:
    enum errors { NONE, TOO_BIG, SHORT_READ, NEW_FAILED };
    errors error;
    MQTTCon(const char *mqttHost, uint16_t mqttPort, const char *caFile,
            const char *certFile, const char *keyFile);

   private:
    X509List *caCert, *clientCert;
    PrivateKey *clientKey;

    bool getFile(const char *file, char buffer[MQTTCON_BUFFER_SIZE]);
    bool getCert(const char *file, X509List *list);
    bool getKey(const char *file, PrivateKey *key);
};

#endif  // MQTTCON_H
