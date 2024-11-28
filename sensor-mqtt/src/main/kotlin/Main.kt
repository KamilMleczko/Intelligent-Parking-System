package parking;

import parking.mqtt.Action
import parking.mqtt.SensorMqttClient
import parking.mqtt.Topic

fun main() {
    val brokerUrl = "tcp://localhost:1883"
    val clientId = "KotlinClient"
    val topic = Topic("home", "garage", "0", Action.HEALTH).toString();
    val mqttClient = SensorMqttClient(brokerUrl, clientId)
    mqttClient.subscribe(topic)
    while (true){
        mqttClient.publish(topic, "Hello!!!!")
        Thread.sleep(1000)
    }
}
