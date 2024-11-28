package parking.mqtt

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.MqttCallback
import org.eclipse.paho.client.mqttv3.MqttMessage

class SensorMqttCallback : MqttCallback{
    override fun connectionLost(cause: Throwable?) {
        println("Connection lost: ${cause?.message}")
    }

    override fun messageArrived(topic: String?, message: MqttMessage?) {
        println("Message arrived: $topic: $message")
    }

    override fun deliveryComplete(token: IMqttDeliveryToken?) {
        println("Message delivered")
    }
}