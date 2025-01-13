package parking.mqtt

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.MqttCallback
import org.eclipse.paho.client.mqttv3.MqttMessage
import parking.api.FirestoreClient
import java.util.concurrent.Executors

class SensorMqttCallback : MqttCallback {

    private fun saveEvent(topic: Topic, event: Event) {
        val firestore = FirestoreClient.getFirestore()
        try {

            val documentRef = firestore.collection("events")
                .document(topic.deviceName)
                .collection("logs")
                .document(event.timestamp.toString())


            val data = mapOf(
                "event" to event.event,
                "timestamp" to event.timestamp,
                "current_people" to event.current_people
            )
            println("will add $data")
            val future = documentRef.set(data) // Returns ApiFuture
            println("saved $data")
            future.addListener(
                {
                    try {
                        println("Event added successfully with ID: ${documentRef.id}")
                    } catch (e: Exception) {
                        println("Failed to add event: ${e.message}")
                    }
                },
                Executors.newSingleThreadExecutor()
            )
        } catch (e: Exception) {
            println("got ex $e")
        }
    }

    override fun connectionLost(cause: Throwable?) {
        println("Connection lost: ${cause?.message}")
    }

    override fun messageArrived(topicString: String, message: MqttMessage?) {
        val topic = Topic.fromString(topicString)
        if (message == null) return
        when (topic.action) {
            Action.HEALTH -> {
                println("Health check")
            }

            Action.STATUS -> {
                println("Status check")
            }

            Action.EVENT -> {
                val event = deserialize(message.toString(), Event::class).getOrNull()
                if (event != null) {
                    saveEvent(topic, event)
                }

            }
        }

    }

    override fun deliveryComplete(token: IMqttDeliveryToken?) {
        println("Message delivered")
    }
}