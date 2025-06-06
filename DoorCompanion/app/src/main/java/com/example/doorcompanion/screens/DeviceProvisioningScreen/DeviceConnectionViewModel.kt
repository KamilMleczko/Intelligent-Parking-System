package com.example.doorcompanion.screens.DeviceProvisioningScreen

import android.content.Context
import android.util.Log
import androidx.lifecycle.ViewModel
import com.espressif.provisioning.DeviceConnectionEvent
import com.espressif.provisioning.ESPConstants
import com.espressif.provisioning.ESPDevice
import com.espressif.provisioning.ESPProvisionManager
import com.espressif.provisioning.listeners.ProvisionListener
import com.espressif.provisioning.listeners.ResponseListener
import com.example.doorcompanion.BleDevice
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.firestore.ktx.firestore
import com.google.firebase.ktx.Firebase
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.greenrobot.eventbus.EventBus
import org.greenrobot.eventbus.Subscribe
import org.greenrobot.eventbus.ThreadMode
import java.nio.charset.StandardCharsets

enum class ConnectionStatus {
    CONNECTED,
    DISCONNECTED,
    FAILED,
    NOT_ATTEMPTED
}

data class ProvisioningLogParams(
    val text: String,
    val result: ProvisioningStepResult
)

data class ProvisioningStatus(
    var deviceConnected: Boolean = false,
    var provisionedSuccesfully: Boolean = false,
    var finishedProvisioning: Boolean = false,
)

class DeviceConnectionViewModel : ViewModel() {
    private val _connectionStatus = MutableStateFlow<ConnectionStatus>(
        ConnectionStatus.NOT_ATTEMPTED
    )

    private val _provisioningLogs = MutableStateFlow<List<ProvisioningLogParams>>(emptyList())

    val connectionStatus = _connectionStatus.asStateFlow()
    val provisioningLogs = _provisioningLogs.asStateFlow()

    private var espDevice: ESPDevice? = null

    var status = ProvisioningStatus()

    init {
        EventBus.getDefault().register(this)
    }

    fun connectToDevice(context: Context, bleDevice: BleDevice) {
        // If no device yet, create one
        if (espDevice == null) {
            val manager = ESPProvisionManager.getInstance(context)
            espDevice = manager.createESPDevice(
                ESPConstants.TransportType.TRANSPORT_BLE,
                ESPConstants.SecurityType.SECURITY_1
            )
        }

        _connectionStatus.value = ConnectionStatus.NOT_ATTEMPTED

        // Actually connect
        espDevice?.connectBLEDevice(
            bleDevice.device,
            bleDevice.primaryServiceUUID
        )
    }

    fun disconnect() {
        espDevice?.disconnectDevice()
    }

    inner class WifiProvisioningListener : ProvisionListener {

        override fun deviceProvisioningSuccess() {
            addToLogs(
                "Provisioning resulted in a success",
                ProvisioningStepResult.SUCCESS
            )
            status.provisionedSuccesfully = true
            status.finishedProvisioning = true
        }

        override fun wifiConfigApplied() {
            addToLogs(
                "Applied wifi config",
                ProvisioningStepResult.SUCCESS
            )
        }

        override fun createSessionFailed(e: Exception?) {
            status.finishedProvisioning = true

            addToLogs(
                "Create session failed",
                ProvisioningStepResult.FAIL
            )
        }

        override fun onProvisioningFailed(e: Exception?) {
            status.finishedProvisioning = true
            addToLogs(
                "Provisioning Failed",
                ProvisioningStepResult.FAIL
            )
        }

        override fun provisioningFailedFromDevice(failureReason: ESPConstants.ProvisionFailureReason?) {
            status.finishedProvisioning = true
            addToLogs("Provisioning failed from device", ProvisioningStepResult.FAIL)
        }

        override fun wifiConfigApplyFailed(e: Exception?) {
            status.finishedProvisioning = true

            addToLogs("Could not apply wifi config", ProvisioningStepResult.FAIL)
        }

        override fun wifiConfigFailed(e: Exception?) {
            // no idea what it means
            status.finishedProvisioning = true

            addToLogs("Wifi config failed", ProvisioningStepResult.FAIL)
        }

        override fun wifiConfigSent() {
            addToLogs("Wifi config sent", ProvisioningStepResult.SUCCESS)
        }
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    fun onDeviceConnected(event: DeviceConnectionEvent) {
        val eventType = event.eventType
        espDevice
        when (eventType) {
            // These EVENT_DEVICE_* constants are not part of an enum
            // so I wrap them into my custom type, to make them be of an enum type
            // instead of short.
            ESPConstants.EVENT_DEVICE_CONNECTED -> {
                status.deviceConnected = true
                _connectionStatus.value = ConnectionStatus.CONNECTED
            }

            ESPConstants.EVENT_DEVICE_CONNECTION_FAILED ->
                _connectionStatus.value = ConnectionStatus.FAILED

            ESPConstants.EVENT_DEVICE_DISCONNECTED ->
                _connectionStatus.value = ConnectionStatus.DISCONNECTED
        }
    }


    fun addToLogs(text: String, result: ProvisioningStepResult) {
        _provisioningLogs.value += ProvisioningLogParams(text, result)
    }

    fun resetDeviceConnection(context: Context, bleDevice: BleDevice) {
        _connectionStatus.value = ConnectionStatus.NOT_ATTEMPTED
        _provisioningLogs.value = emptyList()
        disconnect()
        connectToDevice(context, bleDevice)
        status = ProvisioningStatus()
    }

    private fun cleanupDeviceBindings(deviceName: String, currentUserId: String) {
        val db = Firebase.firestore
        Log.i("DOOR", "Cleanup started for device: $deviceName")

        // Use a collection group query to search all "devices" subcollections
        db.collectionGroup("devices")
            .whereEqualTo("name", deviceName) // Match the nested "device.name" field
            .get()
            .addOnSuccessListener { devicesSnapshot ->
                if (devicesSnapshot.isEmpty) {
                    Log.i("DOOR", "No devices found with name: $deviceName.")
                    return@addOnSuccessListener
                }

                Log.i("DOOR", "Found ${devicesSnapshot.size()} devices with name: $deviceName.")

                for (deviceDoc in devicesSnapshot.documents) {
                    // Get the parent user ID from the path
                    val parentPath = deviceDoc.reference.path
                    val userId =
                        parentPath.split("/")[1] // Extract userId from "users/{userId}/devices/{docId}"

                    // Skip the current user's devices
                    if (userId == currentUserId) {
                        Log.i("DOOR", "Skipping current user with UID=$userId")
                        continue
                    }

                    // Delete the document
                    deviceDoc.reference.delete()
                        .addOnSuccessListener {
                            Log.i(
                                "DOOR",
                                "Removed device '$deviceName' from user with UID=$userId."
                            )
                        }
                        .addOnFailureListener { e ->
                            Log.e("DOOR", "Failed to remove device for user with UID=$userId: $e")
                        }
                }
            }
            .addOnFailureListener { e ->
                Log.e("DOOR", "Error querying devices collection group: $e")
            }
    }

    private fun saveDeviceToFirebase(params: ProvisioningParams) {
        val currentUser = FirebaseAuth.getInstance().currentUser
        if (currentUser != null) {
            val db = Firebase.firestore
            val userDevicesRef =
                db.collection("users").document(currentUser.uid).collection("devices")

            val deviceData = mapOf(
                "name" to params.deviceName,
                "pop" to params.pop,
                "maxPeople" to params.maxPeople,
                "timestamp" to System.currentTimeMillis()
            )

            // Check if the device already exists
            userDevicesRef
                .whereEqualTo("name", params.deviceName)
                .get()
                .addOnSuccessListener { querySnapshot ->
                    if (!querySnapshot.isEmpty) {
                        // If the device exists, update the existing document
                        val existingDoc = querySnapshot.documents.first()
                        existingDoc.reference.set(deviceData)
                            .addOnSuccessListener {
                                cleanupDeviceBindings(
                                    deviceName = params.deviceName,
                                    currentUser.uid
                                )
                                Log.i(
                                    "DOOR",
                                    "Device updated successfully for '${params.deviceName}'."
                                )
                            }
                            .addOnFailureListener { e ->
                                Log.e("DOOR", "Failed to update device '${params.deviceName}': $e")
                            }
                    } else {
                        // If the device does not exist, create a new document
                        userDevicesRef.add(deviceData)
                            .addOnSuccessListener {
                                cleanupDeviceBindings(
                                    deviceName = params.deviceName,
                                    currentUserId = currentUser.uid
                                )
                                Log.i(
                                    "DOOR",
                                    "Device added successfully for '${params.deviceName}'."
                                )
                            }
                            .addOnFailureListener { e ->
                                Log.e("DOOR", "Failed to add device '${params.deviceName}': $e")
                            }
                    }
                }
                .addOnFailureListener { e ->
                    Log.e("DOOR", "Error querying devices for '${params.deviceName}': $e")
                }
        } else {
            Log.e("DOOR", "No authenticated user found. Cannot bind device.")
        }
    }

    fun doProvisioning(params: ProvisioningParams) {
        val (pop, ssid, password, maxPeople, bindUser) = params
        espDevice?.proofOfPossession = pop
        val maxPeopleAsByteArray = maxPeople.toString().toByteArray(StandardCharsets.UTF_8)
        if (bindUser) {
            Log.i("DOOR", "Will bind device!")
            saveDeviceToFirebase(params)
        }
        espDevice?.sendDataToCustomEndPoint(
            "maxPeople",
            maxPeopleAsByteArray,
            object : ResponseListener {
                override fun onSuccess(returnData: ByteArray?) {
                    if (returnData != null) {
                        val cleanData = returnData.takeWhile { it != 0.toByte() }
                            .toByteArray() // Remove data after null terminator
                        val decodedString =
                            java.lang.String(cleanData, StandardCharsets.UTF_8)
                        Log.i("DOOR", "got data $decodedString")

                    } else {
                        Log.i("DOOR", "No data received")
                    }
                    espDevice?.provision(
                        ssid,
                        password,
                        WifiProvisioningListener()
                    )
                }

                override fun onFailure(e: java.lang.Exception?) {
                    Log.i("DOOR", "error in maxPeopleListener :( ${e}")
                }

            })
    }

    override fun onCleared() {
        super.onCleared()
        EventBus.getDefault().unregister(this)
    }
}
