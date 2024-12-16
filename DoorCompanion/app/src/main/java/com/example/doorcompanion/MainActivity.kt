package com.example.doorcompanion

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.content.Context
import android.content.ContextWrapper
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.core.app.ActivityCompat
import com.espressif.provisioning.ESPConstants
import com.espressif.provisioning.ESPConstants.SecurityType
import com.espressif.provisioning.ESPConstants.TransportType
import com.espressif.provisioning.ESPProvisionManager
import com.espressif.provisioning.listeners.BleScanListener
import com.example.doorcompanion.ui.theme.DoorCompanionTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            DoorCompanionTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Greeting(
                        name = "ppp",
                        modifier = Modifier.padding(innerPadding)
                    )
                    BleScanScreen()
                }
            }
        }
    }
}

fun Context.findActivity(): Activity {
    var context = this
    while (context is ContextWrapper) {
        if (context is Activity) return context
        context = context.baseContext
    }
    throw IllegalStateException("no activity")
}

@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    Text(
        text = "Hello $name!",
        modifier = modifier
    )
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    DoorCompanionTheme {
        Greeting("Android")
    }
}

@SuppressLint("MissingPermission")
@Composable
fun DeviceItem(device: BleDevice, onClick: () -> Unit) {
    Text(
        text = "${device.device.name} (${device.device.address}) | ${device.primaryServiceUUID}",
        modifier = Modifier
            .clickable { onClick() }
            .padding(8.dp)
    )
}

@SuppressLint("MissingPermission")
@Composable
fun BleScanScreen() {
    val context = LocalContext.current
    val manager = remember { ESPProvisionManager.getInstance(context) }

    // State to hold the list of discovered device names
    var discoveredDevices by remember {
        mutableStateOf(setOf<BleDevice>())
    }

    Column(modifier = Modifier.fillMaxSize()) {
        Button(onClick = {
            startBleScan(context, manager) { device ->
                discoveredDevices = discoveredDevices + (device)
            }
        }) {
            Text(text = "Start Scan")
        }

        Text(text = "Discovered Devices:")

        LazyColumn {
            items(discoveredDevices.toList()) { bleDevice ->
                DeviceItem(bleDevice){
                    connectToDevice(context, bleDevice)
                }
            }
        }
    }
}

private fun startBleScan(
    context: Context,
    manager: ESPProvisionManager,
    onDeviceFound: (BleDevice) -> Unit
) {

    val activity = context.findActivity();

    if (ActivityCompat.checkSelfPermission(
            context,
            Manifest.permission.ACCESS_FINE_LOCATION
        ) != PackageManager.PERMISSION_GRANTED
    ) {
        toastNotify(context, "NEED ACCESS_FINE_LOCATION");
        ActivityCompat.requestPermissions(
            activity,
            arrayOf(
                Manifest.permission.ACCESS_FINE_LOCATION,
            ),
            2
        )
    }

    if (ActivityCompat.checkSelfPermission(
            context,
            Manifest.permission.BLUETOOTH_SCAN
        ) != PackageManager.PERMISSION_GRANTED
    ) {
        toastNotify(context, "NEED BLUETOOTH_SCAN");
        ActivityCompat.requestPermissions(
            activity,
            arrayOf(
                Manifest.permission.BLUETOOTH_SCAN,
            ),
            2
        )
    }

    manager.searchBleEspDevices(
        "PROV_",
        EspBleScanListener(context,
            onDeviceFound
            )
        )

}



@SuppressLint("MissingPermission")
private fun connectToDevice(context: Context, device: BleDevice) {
    val manager = ESPProvisionManager.getInstance(context)
    val espDevice = manager.createESPDevice(TransportType.TRANSPORT_BLE, SecurityType.SECURITY_1)
    espDevice.bluetoothDevice = device.device;
    espDevice.proofOfPossession = "abcd1234";

//    val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager;
//    val ver = bluetoothManager.adapter.startDiscovery().
    Log.i("DOOR", "PRIMARY SERVUUID: ${device.primaryServiceUUID}")
    espDevice.connectBLEDevice(espDevice.bluetoothDevice, device.primaryServiceUUID);
    Handler(Looper.getMainLooper()).postDelayed({
        espDevice.provision("Mockinixinternet", "NienienieDlaPanaTo", WifiProvisioningListener(context));
    }, 5000)
}