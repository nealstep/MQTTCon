#ifndef MQTTCON_H
#define MQTTCON_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define MQTTCON_BUFFER_SIZE 2048
#define MQTT_ID_SIZE 16
#define TOPIC_SIZE 32
#define TIME_STR_SIZE 28
#define ALIVE_MSG_SIZE 64
#define MQTT_ERROR_MAX 5

class MQTTCon {
   public:
    enum errors { NONE, TOO_BIG, SHORT_READ, NEW_FAILED, CONNECT_FAILED };
    errors error;
    PubSubClient *mqtt = nullptr;

    MQTTCon(void);
    bool setup(const char *mqttHost, uint16_t mqttPort, const char *caFile,
               const char *certFile, const char *keyFile,
               const char *healthTopic);
    bool loop() {
        if (mqtt) return mqtt->loop();
        return false;
    }
    bool check();
    const char *getID() { return mqttID; }

   private:
#ifdef ESP8266
    X509List *caCert;
    X509List *clientCert;
    PrivateKey *clientKey;
#elif defined(ESP32)
    char *caCert;
    char *clientCert;
    char *clientKey;
#endif  // ESP
    char mqttID[MQTT_ID_SIZE];
    char health[TOPIC_SIZE];
    char timeStr[TIME_STR_SIZE];
    char aliveMsg[ALIVE_MSG_SIZE];

    bool getFile(const char *file, char *buffer, size_t len);
#if defined(ESP8266)
    X509List *getCert(const char *file);
    PrivateKey *getKey(const char *file);
#endif  // ESP8266
    void getTimeStamp(char *buffer, size_t len);
};

#endif  // MQTTCON_H
