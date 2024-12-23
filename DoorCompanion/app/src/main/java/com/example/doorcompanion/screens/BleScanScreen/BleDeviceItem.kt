package com.example.doorcompanion.screens.BleScanScreen

import android.annotation.SuppressLint
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.doorcompanion.BleDevice

@SuppressLint("MissingPermission")
@Composable
fun BleDeviceItem(device: BleDevice, onClick: () -> Unit) {
    Text(
        text = "${device.device.name} (${device.device.address})",
        modifier = Modifier
            .padding(8.dp)
            .clickable { onClick() }
    )
}
