#include "mqtt_utils.h"

#include <mqtt_client.h>

#include "credentials.h"
#include "utils.h"
#include "wifi.h"

/* =============== CONSTANTS ==================== */

#define MQTT_MSG_BUFFER 256
const char* PERSON_ENTERED_MESSAGE = "Person entered";
const char* PERSON_LEFT_MESSAGE = "Person left";

void write_mqtt_topic(char* buffer, const ActionType action) {
  char* prefix = MQTT_TOPIC_PREFIX;

  switch (action) {
    case HEALTH:
      snprintf(buffer, MQTT_MSG_BUFFER, "%s/%s/health", prefix, DEVICE_NAME);
      break;
    case EVENT:
      snprintf(buffer, MQTT_MSG_BUFFER, "%s/%s/event", prefix, DEVICE_NAME);
      break;
    case STATUS:
      snprintf(buffer, MQTT_MSG_BUFFER, "%s/%s/status", prefix, DEVICE_NAME);
      break;
    default:
      snprintf(buffer, MQTT_MSG_BUFFER, "Unknown topic");
      break;
  }
}
// These are loaded during build time
extern const uint8_t server_root_cert_pem_start[] asm(
    "_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[] asm(
    "_binary_server_root_cert_pem_end");

esp_mqtt_client_handle_t mqtt_connect(char* broker_uri, char* username,
                                      char* password) {
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker = {.address = {.uri = broker_uri},
                 .verification.certificate =
                     (const char*)server_root_cert_pem_start},
      .credentials = {.username = username,
                      .authentication = {
                          .password = password,
                      }}};
  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
                                 client);

  esp_mqtt_client_start(client);
  return client;
}

void mqtt_publish_event(esp_mqtt_client_handle_t client, const EventType event,
                        const time_t timestamp, const int current_people) {
  char buffer[MQTT_MSG_BUFFER];
  serialize_event(buffer, event, timestamp, current_people);
  ESP_LOGI(LOG_MQTT, "Publishing event: %s to %s", buffer, MQTT_TOPIC_EVENT);
  esp_mqtt_client_publish(client, MQTT_TOPIC_EVENT, buffer, 0, 1, 0);
}

void serialize_event(char* buffer, const EventType event,
                     const time_t timestamp, const int current_people) {
  snprintf(
      buffer, MQTT_MSG_BUFFER, "event: %s\ntimestamp: %lld\ncurrent_people: %d",
      event == PERSON_ENTERED ? PERSON_ENTERED_MESSAGE : PERSON_LEFT_MESSAGE,
      timestamp, current_people);
}

void serialize_healthcheck(char* buffer) {
  snprintf(buffer, MQTT_MSG_BUFFER,
           "online: true"  // if it weren't alive, then there wouldn't be
                           // anuthing to send, duh
  );
}

void mqtt_publish_healthcheck(esp_mqtt_client_handle_t client) {
  char buffer[MQTT_MSG_BUFFER];
  serialize_healthcheck(buffer);
  esp_mqtt_client_publish(client, MQTT_TOPIC_HEALTHCHECK, buffer, 0, 1, 0);
}

void serialize_status(char* buffer, const StatusType status) {
  snprintf(buffer, MQTT_MSG_BUFFER, "occupied: %s",
           status == FREE ? "true" : "false");
}

void mqtt_publish_status(esp_mqtt_client_handle_t client,
                         const StatusType status) {
  char buffer[MQTT_MSG_BUFFER];
  serialize_status(buffer, status);
  esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, buffer, 0, 1, 0);
}

void mqtt_event_handler(void* handler_args, esp_event_base_t base,
                        int32_t event_id, void* event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  char buffer[MQTT_MSG_BUFFER];
  switch (event_id) {
    case MQTT_EVENT_CONNECTED:
      snprintf(buffer, MQTT_MSG_BUFFER, "\n\n\nconnected!!!!!!\n\n\n");
      printf("%s\n", buffer);
      // Optionally, subscribe to topics or publish messages here
      break;

    case MQTT_EVENT_DISCONNECTED:
      snprintf(buffer, MQTT_MSG_BUFFER, "Disconnected from broker\n");
      printf("%s\n", buffer);
      break;

    case MQTT_EVENT_SUBSCRIBED:
      snprintf(buffer, MQTT_MSG_BUFFER, "Subscribed to topic successfully\n");
      printf("%s\n", buffer);
      break;

    case MQTT_EVENT_UNSUBSCRIBED:
      snprintf(buffer, MQTT_MSG_BUFFER, "Unsubscribed from topic\n");
      printf("%s\n", buffer);
      break;

    case MQTT_EVENT_PUBLISHED:
      snprintf(buffer, MQTT_MSG_BUFFER, "Message published successfully!\n");
      printf("%s\n", buffer);
      break;

    case MQTT_EVENT_DATA:
      snprintf(buffer, MQTT_MSG_BUFFER, "Received data on topic: %.*s\n",
               event->topic_len, event->topic);
      printf("%s\n", buffer);
      snprintf(buffer, MQTT_MSG_BUFFER, "Data: %.*s\n", event->data_len,
               event->data);
      printf("%s\n", buffer);
      break;

    case MQTT_EVENT_BEFORE_CONNECT:
      snprintf(buffer, MQTT_MSG_BUFFER, "Before connecting to broker\n");
      printf("%s\n", buffer);
      break;

    case MQTT_EVENT_ERROR:
      snprintf(buffer, MQTT_MSG_BUFFER, "An error occurred\n");
      printf("%s\n", buffer);
      break;

    default:
      snprintf(buffer, MQTT_MSG_BUFFER, "Unhandled event ID: %ld\n", event_id);
      printf("%s\n", buffer);
      break;
  }
}

char MQTT_TOPIC_HEALTHCHECK_BUFFER[MQTT_TOPIC_MAX_LENGTH];
char MQTT_TOPIC_STATUS_BUFFER[MQTT_TOPIC_MAX_LENGTH];
char MQTT_TOPIC_EVENT_BUFFER[MQTT_TOPIC_MAX_LENGTH];

const char* MQTT_TOPIC_HEALTHCHECK = MQTT_TOPIC_HEALTHCHECK_BUFFER;
const char* MQTT_TOPIC_STATUS = MQTT_TOPIC_STATUS_BUFFER;
const char* MQTT_TOPIC_EVENT = MQTT_TOPIC_EVENT_BUFFER;

void init_mqtt_topics(void) {
  write_mqtt_topic(MQTT_TOPIC_HEALTHCHECK_BUFFER, HEALTH);
  write_mqtt_topic(MQTT_TOPIC_STATUS_BUFFER, STATUS);
  write_mqtt_topic(MQTT_TOPIC_EVENT_BUFFER, EVENT);
}