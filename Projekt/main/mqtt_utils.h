#include <mqtt_client.h>
#include <time.h>

#ifndef MQTT_UTILS_H
#define MQTT_UTILS_H

extern const char* PERSON_ENTERED_MESSAGE;
extern const char* PERSON_LEFT_MESSAGE;
extern const char* MQTT_TOPIC_HEALTHCHECK;
extern const char* MQTT_TOPIC_STATUS;
extern const char* MQTT_TOPIC_EVENT;

#define MQTT_TOPIC_MAX_LENGTH 128

typedef enum { HEALTH = 0, EVENT = 1, STATUS = 2 } ActionType;

typedef enum { PERSON_ENTERED = 0, PERSON_LEFT = 1 } EventType;

typedef enum { FREE = 0, OCCUPIED = 1 } StatusType;

esp_mqtt_client_handle_t mqtt_connect(char* broker_uri, char* username,
                                      char* password);

void serialize_event(char* buffer, const EventType event,
                     const time_t timestamp, const int current_people);

void mqtt_publish_event(esp_mqtt_client_handle_t client, const EventType event,
                        const time_t timestamp, const int current_people);

void serialize_healthcheck(char* buffer);

void mqtt_publish_healthcheck(esp_mqtt_client_handle_t client);

void serialize_statuss(char* buffer, const StatusType status);

void mqtt_publish_status(esp_mqtt_client_handle_t client,
                         const StatusType status);

void mqtt_event_handler(void* handler_args, esp_event_base_t base,
                        int32_t event_id, void* event_data);
void init_mqtt_topics(void);

#endif  // MQTT_UTILS_H
