package com.example.doorcompanion.screens.BleScanScreen

import Screen
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
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

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = androidx.compose.foundation.layout.Arrangement.spacedBy(16.dp)
    ) {
        // Go to Stats Button
        CenteredButton(
            text = "Go to Stats",
            onClick = { router.navigateTo(Screen.Stats) }
        )

        // Start Scan Button
        CenteredButton(
            text = "Start Scan",
            onClick = { viewModel.startBleScan(context) }
        )

        // Discovered Devices Section
        Text(
            text = "Discovered Devices:",
            style = androidx.compose.material3.MaterialTheme.typography.headlineMedium,
            modifier = Modifier.padding(vertical = 16.dp)
        )
        BleDevicesList(
            discoveredDevices,
            onDeviceSelected = { device ->
                router.navigateTo(Screen.DeviceProvisioning(device))
            }
        )
    }
}

// Centered Button Composable for Reusability
@Composable
fun CenteredButton(
    text: String,
    onClick: () -> Unit,
) {
    androidx.compose.foundation.layout.Box(
        modifier = Modifier.fillMaxWidth(),
        contentAlignment = androidx.compose.ui.Alignment.Center
    ) {
        Button(
            onClick = onClick,
            modifier = Modifier
                .padding(horizontal = 16.dp)
                .height(56.dp)
                .fillMaxWidth(0.8f), // Makes the button width 80% of the parent
            shape = androidx.compose.foundation.shape.RoundedCornerShape(50)
        ) {
            Text(
                text = text,
                style = androidx.compose.material3.MaterialTheme.typography.bodyLarge
            )
        }
    }
}
