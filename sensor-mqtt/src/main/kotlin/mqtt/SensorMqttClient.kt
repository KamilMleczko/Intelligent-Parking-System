package parking.mqtt

import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttException

class SensorMqttClient(
    private val brokerUrl: String,
    private val clientId: String,
    private val username: String? = null,
    private val passWord: String? = null
) {
    private var mqttClient: MqttClient = MqttClient(brokerUrl, clientId);

    init {
        connect();
    }

    private fun connect(){
        val options = MqttConnectOptions().apply {
            isCleanSession = true
            username?.let { userName = it }
            passWord?.let { password = it.toCharArray() }
        }
        mqttClient = MqttClient(brokerUrl, clientId);
        mqttClient.setCallback(SensorMqttCallback())
        try {
            mqttClient.connect(options)
            println("Succesfully connected to $brokerUrl")
        }catch (ex: MqttException){
            println("Failed to connect to $brokerUrl")
        }
    }

    fun subscribe(topic: String) {
        try {
            mqttClient.subscribe(topic)
            println("Subscribed to $topic")
        } catch (ex: MqttException) {
            println("Failed to subscribe $topic, error: ${ex.message}")
        }
    }

    fun publish(topic: String, message: String) {
        val mqttMessage = MqttMessage(message.toByteArray())
        try {
            mqttClient.publish(topic, mqttMessage)
            println("Published $topic to $mqttMessage")
        }catch (ex: MqttException) {
            println("Failed to publish $topic to $mqttMessage, error: ${ex.message}")
        }
    }

}
