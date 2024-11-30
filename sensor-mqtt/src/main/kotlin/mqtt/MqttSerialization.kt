package parking.mqtt

import kotlinx.serialization.*
import kotlinx.serialization.json.*

@Serializable
data class Health(
    val online: Boolean
)

@Serializable
data class Event(
    val event: String, // "car parked" | "car departed"
    val timestamp: Long
) {
    init {
        require(event in setOf("car parked", "car departed")) { "Invalid event type: $event" }
        require(timestamp > 0) { "Timestamp must be a positive integer" }
    }
}

@Serializable
data class Status(
    val occupied: Boolean
)
