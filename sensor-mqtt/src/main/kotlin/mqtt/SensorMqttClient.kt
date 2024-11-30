package parking.mqtt

import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttException
import java.security.KeyStore
import javax.net.ssl.SSLContext
import javax.net.ssl.TrustManagerFactory

class SensorMqttClient(
    private val brokerUrl: String,
    private val clientId: String,
    private val username: String = "",
    private val passWord: String = ""
) {
    private var mqttClient: MqttClient = MqttClient(brokerUrl, clientId);

    init {
        connect();
    }

    private fun getSslContext(): SSLContext {
        val trustStore = KeyStore.getInstance(KeyStore.getDefaultType())
        trustStore.load(null)

        val trustManagerFactory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm())
        trustManagerFactory.init(null as KeyStore?)

        val sslContext = SSLContext.getInstance("TLS")
        sslContext.init(null, trustManagerFactory.trustManagers, null)
        return sslContext
    }

    private fun connect(){
        val sslContext = getSslContext();
        val options = MqttConnectOptions().apply {
            isCleanSession = true
            socketFactory = sslContext.socketFactory
            username.let { userName = it }
            passWord.let { password = it.toCharArray() }
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
