package parking.mqtt

import java.io.FileInputStream
import java.util.*

data class MqttProperties(
    val brokerUrl: String,
    val clientId: String = "KotlinClient",
    val username: String = "",
    val password: String = "",
    val organisation: String = "",
    val location: String = ""
);

fun loadMqttProperties(filePath: String): MqttProperties {
    val props = Properties();
    FileInputStream(filePath).use {
        props.load(it)
    }
    return MqttProperties(
        brokerUrl = props.getProperty("brokerUrl"),
        clientId = props.getProperty("clientId"),
        username = props.getProperty("username"),
        password = props.getProperty("password"),
        organisation = props.getProperty("organisation"),
        location = props.getProperty("location"),
    )

}