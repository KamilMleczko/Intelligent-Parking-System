package com.example.doorcompanion.screens.BleScanScreen

import Screen
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.doorcompanion.routing.Router


@Composable
fun BleScanScreen(
    router: Router,
    viewModel: BleScanViewModel = viewModel(),

    ) {
    val context = LocalContext.current
    val discoveredDevices by viewModel.discoveredDevices.collectAsState()

    Column(modifier = Modifier.fillMaxSize()) {

        Spacer(Modifier.padding(8.dp))

        Button(onClick = {
            router.navigateTo(Screen.Stats)
        }) {
            Text(text = "Go to stats")
        }

        Button(onClick = {
            viewModel.startBleScan(
                context,
            )
        }) {
            Text(text = "Start Scan")
        }


        Text(text = "Discovered Devices:")
        BleDevicesList(
            discoveredDevices,
            onDeviceSelected = { device ->
                router.navigateTo(Screen.DeviceProvisioning(device))
            }
        )
    }
}


