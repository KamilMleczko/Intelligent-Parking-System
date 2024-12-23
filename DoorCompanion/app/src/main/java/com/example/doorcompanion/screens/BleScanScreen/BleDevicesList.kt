package com.example.doorcompanion.screens.BleScanScreen

import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.runtime.Composable
import com.example.doorcompanion.BleDevice

@Composable
fun BleDevicesList(
    discoveredDevices: Set<BleDevice>,
    onDeviceSelected: (BleDevice) -> Unit
) {
    LazyColumn {
        items(discoveredDevices.toList()) { bleDevice ->
            // This is just a sample representation.
            BleDeviceItem(bleDevice) {
                // Connect to this device
                onDeviceSelected(
                    bleDevice
                )
            }
        }
    }
}
