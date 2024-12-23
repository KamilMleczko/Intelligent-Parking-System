package com.example.doorcompanion.screens.BleScanScreen

import android.Manifest
import android.bluetooth.BluetoothDevice
import android.bluetooth.le.ScanResult
import android.content.Context
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat
import com.espressif.provisioning.listeners.BleScanListener
import com.example.doorcompanion.BleDevice
import com.example.doorcompanion.findActivity
import com.example.doorcompanion.toastNotify


class EspBleScanListener(
    private val context: Context,
    val onDeviceFound: (BleDevice) -> Unit,
) : BleScanListener {


    override fun scanCompleted() {
        toastNotify(context, "Scan completed!")
    }

    override fun scanStartFailed() {
        toastNotify(context, "Scan failed")
    }

    override fun onFailure(e: Exception?) {
        toastNotify(context, "Scan failed")
    }

    override fun onPeripheralFound(device: BluetoothDevice, scanResult: ScanResult) {
        if (ActivityCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH_CONNECT
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            toastNotify(context, "BLUETOOTH_CONNECT - provide")
            ActivityCompat.requestPermissions(
                context.findActivity(),
                arrayOf(
                    Manifest.permission.BLUETOOTH_CONNECT
                ),
                1
            )
        }
        val uuids = scanResult.scanRecord?.serviceUuids

        if (uuids != null && uuids.size > 0) {
            onDeviceFound(BleDevice(device, uuids[0].toString()))
        }
    }
}