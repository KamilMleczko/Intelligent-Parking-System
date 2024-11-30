#include <time.h>
#include <mqtt_client.h>

#ifndef MQTT_UTILS_H
#define MQTT_UTILS_H
extern const char* CAR_PARKED_MESSAGE;
extern const char* CAR_LEFT_MESSAGE;
extern const char* MQTT_TOPIC_HEALTHCHECK;
extern const char* MQTT_TOPIC_STATUS;
extern const char* MQTT_TOPIC_EVENT;
#endif // MQTT_UTILS_H

typedef enum {
  HEALTH = 0,
  EVENT = 1,
  STATUS = 2
} ActionType;

typedef enum {
    CAR_PARKED = 0,
    CAR_LEFT = 1
} EventType;


typedef enum {
  FREE = 0,
  OCCUPIED = 1
} StatusType;


esp_mqtt_client_handle_t mqtt_connect(char* broker_uri, char* username, char* password);


void serialize_event(
  char* buffer, 
  const EventType event, 
  const time_t timestamp
);

void mqtt_publish_event(
  esp_mqtt_client_handle_t client,
  const EventType event, 
  const time_t timestamp
);

void serialize_healthcheck(
  char* buffer
);

void mqtt_publish_healthcheck(
  esp_mqtt_client_handle_t client
);

void serialize_statuss(
  char* buffer,
  const StatusType status
);

void mqtt_publish_status(
  esp_mqtt_client_handle_t client,
  const StatusType status
);


void mqtt_event_handler(
  void *handler_args, 
  esp_event_base_t base, 
  int32_t event_id, 
  void *event_data
);
