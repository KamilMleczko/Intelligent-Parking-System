package parking.mqtt

import kotlinx.serialization.Serializable

@Serializable
data class Health(
    val online: Boolean
)

@Serializable
data class Event(
    val event: String,
    val timestamp: Long,
    val current_people: Int,
) {
    init {
        require(event in setOf("Person left", "Person entered")) { "Invalid event type: $event" }
        require(timestamp > 0) { "Timestamp must be a positive integer" }
    }
}

@Serializable
data class Status(
    val occupied: Boolean
)
