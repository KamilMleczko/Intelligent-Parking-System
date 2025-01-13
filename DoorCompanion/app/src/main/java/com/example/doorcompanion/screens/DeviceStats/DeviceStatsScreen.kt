package com.example.doorcompanion.screens.DeviceStats

import DeviceStatsViewModel
import Screen
import androidx.compose.foundation.layout.Column
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.doorcompanion.routing.Router

@Composable
fun DeviceStatsScreen(router: Router, devicesViewModel: DeviceStatsViewModel = viewModel()) {
    val devices = devicesViewModel.devices.collectAsState().value

    Text(
        "Currently tracking ${devices.count()} devices!"
    )

    Column {
        Text("Devices", style = MaterialTheme.typography.headlineSmall)

        devices.forEach { device ->
            Button(
                onClick = {
                    router.navigateTo(Screen.DeviceSummary(device = device))
                }
            ) {
                Text("Device: ${device.name}, Max People: ${device.maxPeople}")
            }
        }
    }
}
