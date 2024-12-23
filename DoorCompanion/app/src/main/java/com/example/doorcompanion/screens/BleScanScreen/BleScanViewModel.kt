package com.example.doorcompanion.screens.BleScanScreen

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat
import androidx.lifecycle.ViewModel
import com.espressif.provisioning.ESPProvisionManager
import com.example.doorcompanion.BleDevice
import com.example.doorcompanion.findActivity
import com.example.doorcompanion.toastNotify
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update

/**
 * Class used to persist set of found BluetoothDevices due to a BLE scan.
 * Aforementioned state has to be stored in ViewModel to persist configuration
 * changes, e.g., due to screen rotation.
 */
class BleScanViewModel : ViewModel() {
    private val _discoveredDevices = MutableStateFlow<Set<BleDevice>>(emptySet())
    val discoveredDevices = _discoveredDevices.asStateFlow()

    private val scanPrefix = "" // constant for now.

    private fun addDevice(device: BleDevice) {
        _discoveredDevices.update { devices ->
            devices + device
        }
    }

    fun onDeviceFound(device: BleDevice) {
        addDevice(device)
    }

    /**
     * Starts scanning for BLE scan with given prefix.
     * @param context - current app context
     */
    fun startBleScan(
        context: Context,
    ) {
        // First; clear currently saved devices.
        _discoveredDevices.update { emptySet() }

        val activity = context.findActivity()

        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.ACCESS_FINE_LOCATION)
            != PackageManager.PERMISSION_GRANTED
        ) {
            toastNotify(context, "NEED ACCESS_FINE_LOCATION")
            ActivityCompat.requestPermissions(
                activity,
                arrayOf(Manifest.permission.ACCESS_FINE_LOCATION),
                2
            )
        }

        if (ActivityCompat.checkSelfPermission(context, Manifest.permission.BLUETOOTH_SCAN)
            != PackageManager.PERMISSION_GRANTED
        ) {
            toastNotify(context, "NEED BLUETOOTH_SCAN")
            ActivityCompat.requestPermissions(
                activity,
                arrayOf(Manifest.permission.BLUETOOTH_SCAN),
                2
            )
        }

        ESPProvisionManager.getInstance(context).searchBleEspDevices(
            scanPrefix,
            EspBleScanListener(context, this::onDeviceFound)
        )
    }

    override fun onCleared() {
        _discoveredDevices.update { emptySet() }
    }
}