package parking.mqtt
enum class Action(val value: String) {
    STATUS("status"),
    EVENT("event"),
    HEALTH("health");

    override fun toString(): String = value
}
/**
 *  Class representing MQTT Topic.
 *  The topic is formatted like so: organisation/location/spotId/action
 *  Parameters represent topic groups.
 *  @param organisation - top level topic group, organisation.
 *  @param location - location withing organisation
 *  @param spotId - id of the spot
 *  @param action - action, i.e car parkings and departures.
 *
 * */
class Topic(
    val organisation: String,
    val location: String,
    val spotId: String,
    val action: Action
) {
    override fun toString(): String {
        return "$organisation/$location/$spotId/$action"
    }
}