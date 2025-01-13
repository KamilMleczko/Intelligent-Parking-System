package parking;

import com.google.auth.oauth2.GoogleCredentials
import com.google.firebase.FirebaseApp
import com.google.firebase.FirebaseOptions
import parking.mqtt.Action
import parking.mqtt.SensorMqttClient
import parking.mqtt.Topic
import parking.mqtt.loadMqttProperties
import java.io.FileInputStream


fun main() {

    val serviceAccount =
        FileInputStream("config/ServiceAccount.json")

    val options: FirebaseOptions = FirebaseOptions.builder()
        .setCredentials(GoogleCredentials.fromStream(serviceAccount))
        .build()

    FirebaseApp.initializeApp(options)


    val props = loadMqttProperties("config/mqtt.properties");
    val (
        brokerUrl,
        clientId,
        username,
        password,
        organisation,
        location
    ) = props;
    val topicHealth = Topic(organisation, location, "+", Action.HEALTH).toString();
    val topicEvent = Topic(organisation, location, "+", Action.EVENT).toString()
    val topicStatus = Topic(organisation, location, "+", Action.STATUS).toString()
    val mqttClient = SensorMqttClient(brokerUrl, clientId, username, password)
    mqttClient.subscribe(topicHealth);
    mqttClient.subscribe(topicStatus);
    mqttClient.subscribe(topicEvent);
}
