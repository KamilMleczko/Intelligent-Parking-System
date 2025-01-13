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
    data class Success(val reading: RoomReading) : RoomReadingStatus()
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
        Log.i("DOOR", "searching @ $deviceName")
        _roomReading.value = RoomReadingStatus.Loading
        logsCollectionRef.addSnapshotListener { snapshot, error ->
            if (error != null) {
                Log.e("DOOR", "Error fetching data: ", error)
                _roomReading.value = RoomReadingStatus.Error(
                    error.message ?: "Something went wrong when fetching reading data"
                )
                return@addSnapshotListener
            }
            Log.i("DOOR", "got ${snapshot?.documents?.lastOrNull()}")
            val responseAsClass =
                snapshot?.documents?.lastOrNull()?.toObject(RoomReading::class.java)
            if (responseAsClass == null) {
                _roomReading.value = RoomReadingStatus.Error("Could not get data for the device!")
                return@addSnapshotListener
            }
            _roomReading.value = RoomReadingStatus.Success(responseAsClass)
        }
    }

}
