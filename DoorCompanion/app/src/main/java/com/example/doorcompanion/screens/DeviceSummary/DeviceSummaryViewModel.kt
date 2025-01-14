import android.util.Log
import androidx.lifecycle.ViewModel
import com.google.firebase.firestore.FirebaseFirestore
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

data class RoomReading(
    val event: String = "",
    val timestamp: Long = 0L,
    val current_people: Int = 0
)

sealed class RoomReadingStatus {
    data object None : RoomReadingStatus()
    data object Loading : RoomReadingStatus()
    data class Success(val readings: List<RoomReading>) :
        RoomReadingStatus() // Updated to hold a list

    data class Error(val message: String) : RoomReadingStatus()
}

class DeviceSummaryViewModel : ViewModel() {

    private val firestore = FirebaseFirestore.getInstance()

    private val _roomReading = MutableStateFlow<RoomReadingStatus>(RoomReadingStatus.None)
    val roomReading: StateFlow<RoomReadingStatus> = _roomReading.asStateFlow()

    fun observeRoomData(deviceName: String) {
        val logsCollectionRef = firestore.collection("events")
            .document(deviceName)
            .collection("logs")

        val now = System.currentTimeMillis()
        val oneDayAgo = (now - 24 * 60 * 60 * 1000) / 1000 // 24 hours ago in seconds

        Log.i("DOOR", "Fetching data for last 24 hours for device: $deviceName")
        _roomReading.value = RoomReadingStatus.Loading

        logsCollectionRef
            .whereGreaterThanOrEqualTo("timestamp", oneDayAgo) // Filter by timestamp
            .orderBy(
                "timestamp",
                com.google.firebase.firestore.Query.Direction.ASCENDING
            )
            .addSnapshotListener { snapshot, error ->
                if (error != null) {
                    Log.e("DOOR", "Error fetching data: ", error)
                    _roomReading.value = RoomReadingStatus.Error(
                        error.message ?: "Something went wrong when fetching reading data"
                    )
                    return@addSnapshotListener
                }

                val readings =
                    snapshot?.documents?.mapNotNull { it.toObject(RoomReading::class.java) }
                if (readings.isNullOrEmpty()) {
                    _roomReading.value =
                        RoomReadingStatus.Error("No data available for the last 24 hours!")
                    return@addSnapshotListener
                }

                Log.i("DOOR", "Successfully fetched ${readings.size} readings.")
                _roomReading.value =
                    RoomReadingStatus.Success(readings) // Store the list of readings

            }
    }
}
