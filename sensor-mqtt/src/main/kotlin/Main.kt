package parking;

import parking.mqtt.SensorMqttClient

fun main() {
    val brokerUrl = "tcp://localhost:1883"
    val clientId = "KotlinClient"
    val topic = "test/topic"
    val mqttClient = SensorMqttClient(brokerUrl, clientId)
    mqttClient.subscribe(topic)
    while (true){
        mqttClient.publish(topic, "Hello!!!!")
        Thread.sleep(1000)
    }
}
