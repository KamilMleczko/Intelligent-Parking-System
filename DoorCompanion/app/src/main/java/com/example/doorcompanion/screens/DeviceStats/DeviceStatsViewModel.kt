import androidx.lifecycle.ViewModel
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.firestore.FirebaseFirestore
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

data class Device(
    val pop: String = "",
    val ssid: String = "",
    val maxPeople: Int = 0,
    val timestamp: Long = 0L
)


class DeviceStatsViewModel : ViewModel() {
    private val _devices = MutableStateFlow<List<Device>>(emptyList())
    val devices: StateFlow<List<Device>> = _devices

    private val firestore = FirebaseFirestore.getInstance()
    private val auth = FirebaseAuth.getInstance()

    init {
        fetchDevices()
    }

    private fun fetchDevices() {
        val currentUser = auth.currentUser
        if (currentUser != null) {
            val userDevicesRef =
                firestore.collection("users").document(currentUser.uid).collection("devices")

            userDevicesRef.addSnapshotListener { snapshot, exception ->
                if (exception != null) {
                    // Handle exception
                    return@addSnapshotListener
                }
                if (snapshot != null) {
                    val deviceList = snapshot.documents.mapNotNull { doc ->
                        doc.toObject(Device::class.java)
                    }
                    _devices.value = deviceList
                }
            }
        }
    }
}
