package parking.mqtt

enum class Action(val value: String) {
    STATUS("status"),
    EVENT("event"),
    HEALTH("health");

    override fun toString(): String = value

    companion object {
        fun fromString(value: String): Action? {
            return entries.find { it.value == value }
        }
    }
}

/**
 *  Class representing MQTT Topic.
 *  The topic is formatted like so: organisation/location/spotId/action
 *  Parameters represent topic groups.
 *  @param organisation - top level topic group, organisation.
 *  @param location - location withing organisation
 *  @param deviceName - deviceName assigned to the device
 *  @param action - action, i.e car parkings and departures.
 *
 * */
class Topic(
    val organisation: String,
    val location: String,
    val deviceName: String,
    val action: Action
) {
    override fun toString(): String {
        return "$organisation/$location/$deviceName/$action"
    }

    companion object {

        fun fromString(topicAsString: String): Topic {
            val splitResult = topicAsString.split('/')
            if (splitResult.size != 4) {
                throw IllegalArgumentException("Invalid topic format: $topicAsString")
            }
            val organisation = splitResult[0]
            val location = splitResult[1]
            val deviceName = splitResult[2]
            val action = splitResult[3]
            val actionFromString = Action.fromString(action)
            if (actionFromString != null) {
                return Topic(organisation, location, deviceName, actionFromString)
            }
            throw IllegalArgumentException("Invalid topic format: $topicAsString")
        }


    }


}