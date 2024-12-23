package com.example.doorcompanion.screens.DeviceProvisioningScreen

import android.content.Context
import androidx.lifecycle.ViewModel
import com.espressif.provisioning.DeviceConnectionEvent
import com.espressif.provisioning.ESPConstants
import com.espressif.provisioning.ESPDevice
import com.espressif.provisioning.ESPProvisionManager
import com.espressif.provisioning.listeners.ProvisionListener
import com.example.doorcompanion.BleDevice
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.greenrobot.eventbus.EventBus
import org.greenrobot.eventbus.Subscribe
import org.greenrobot.eventbus.ThreadMode

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

    fun doProvisioning(params: ProvisioningParams) {
        val (pop, ssid, password) = params
        espDevice?.proofOfPossession = pop
        espDevice?.provision(
            ssid,
            password,
            WifiProvisioningListener()
        )
    }

    override fun onCleared() {
        super.onCleared()
        EventBus.getDefault().unregister(this)
    }
}
