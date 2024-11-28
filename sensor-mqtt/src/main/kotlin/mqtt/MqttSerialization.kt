package parking.mqtt

import kotlinx.serialization.*
import kotlinx.serialization.json.*

@Serializable
data class Health(
    val battery: Float
) {
    init {
        require(battery in 0.0..100.0) { "Battery must be within valid range (0.0 to 100.0)" }
    }
}

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
