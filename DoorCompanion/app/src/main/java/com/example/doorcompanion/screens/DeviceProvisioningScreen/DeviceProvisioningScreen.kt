package com.example.doorcompanion.screens.DeviceProvisioningScreen

import android.annotation.SuppressLint
import android.util.Log
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.platform.LocalContext
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.doorcompanion.BleDevice
import com.example.doorcompanion.LoadingSpinner

@SuppressLint("MissingPermission")
@Composable
fun DeviceProvisioningScreen(
    bleDevice: BleDevice,
    viewModel: DeviceConnectionViewModel = viewModel()
) {
    val connectionStatus = viewModel.connectionStatus.collectAsState().value
    val logs = viewModel.provisioningLogs.collectAsState().value
    val context = LocalContext.current


    val status = viewModel.status

    DisposableEffect(bleDevice) {
        Log.i("DOOR", "connecting to BLE device")
        viewModel.connectToDevice(context, bleDevice)
        onDispose {
            Log.i("DOOR", "disconnecting from BLE device.")
            viewModel.disconnect()
        }
    }


    Text("Provision device")

    when (connectionStatus) {
        ConnectionStatus.NOT_ATTEMPTED ->
            LoadingSpinner("Waiting for connection with device...")

        ConnectionStatus.DISCONNECTED ->
            Text("Connection with device has been terminated.")

        ConnectionStatus.FAILED ->
            Text("Failed connecting with device.")

        ConnectionStatus.CONNECTED -> {
            if (!status.provisionedSuccesfully) {
                DeviceProvisioningForm(bleDevice) { params ->
                    viewModel.doProvisioning(params)
                }
            }
        }
    }

    ProvisioningLogs(logs)
    if (status.provisionedSuccesfully) {
        AwesomeSuccess()
    }
    Text("Status: ${connectionStatus.name}")
    Text("Status: ${connectionStatus.name}")
    if (!status.provisionedSuccesfully && status.finishedProvisioning) {
        Button(
            onClick = {
                viewModel.resetDeviceConnection(context, bleDevice)
            }
        ) {
            Text("Retry")
        }
    }

}
