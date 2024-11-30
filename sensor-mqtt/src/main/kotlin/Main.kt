package parking;

import parking.mqtt.*

fun main() {
    val props = loadMqttProperties("config/mqtt.properties");
    val (
        brokerUrl,
        clientId,
        username,
        password
    ) = props;
    val topicHealth = Topic("home", "garage", "0", Action.HEALTH).toString();
    val topicEvent = Topic("home", "garage", "0", Action.EVENT).toString()
    val topicStatus = Topic("home", "garage", "0", Action.STATUS).toString()
    val mqttClient = SensorMqttClient(brokerUrl, clientId, username, password)
    mqttClient.subscribe(topicHealth);
    mqttClient.subscribe(topicStatus);
    mqttClient.subscribe(topicEvent);
}
