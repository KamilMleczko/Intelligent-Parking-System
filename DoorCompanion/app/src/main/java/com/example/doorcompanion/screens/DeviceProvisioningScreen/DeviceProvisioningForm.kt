package com.example.doorcompanion.screens.DeviceProvisioningScreen

import android.annotation.SuppressLint
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import com.example.doorcompanion.BleDevice

data class ProvisioningParams(
    val pop: String,
    val ssid: String,
    val password: String,
    val maxPeople: Int,
    val bindDevice: Boolean,
    val deviceName: String
)

@SuppressLint("MissingPermission")
@Composable
fun DeviceProvisioningForm(
    device: BleDevice,
    onSubmit: (ProvisioningParams) -> Unit
) {
    var pop by remember { mutableStateOf("abcd1234") }
    var ssid by remember { mutableStateOf("Logiczna Sieć") }
    var password by remember { mutableStateOf("srzj6042") }
    var maxPeople by remember { mutableIntStateOf(30) }
    var bindDevice by remember { mutableStateOf(false) }

    Column(modifier = Modifier.padding(16.dp)) {

        Spacer(modifier = Modifier.padding(8.dp))

        OutlinedTextField(
            value = pop,
            onValueChange = { pop = it },
            label = { Text("Proof of Possession") }
        )

        Spacer(modifier = Modifier.padding(8.dp))

        OutlinedTextField(
            value = ssid,
            onValueChange = { ssid = it },
            label = { Text("Wi-Fi SSID") }
        )

        Spacer(modifier = Modifier.padding(8.dp))
        OutlinedTextField(
            value = password,
            onValueChange = { password = it },
            label = { Text("Wi-Fi Password") }
        )

        Spacer(modifier = Modifier.padding(8.dp))
        OutlinedTextField(
            value = maxPeople.toString(),
            onValueChange = { input ->
                // Allow only numeric input
                if (input.all { it.isDigit() }) {
                    maxPeople = if (input.isEmpty()) 0 else input.toInt()
                }
            },
            label = { Text("Max amount of people") },
            keyboardOptions = KeyboardOptions(
                keyboardType = KeyboardType.Number
            )
        )


        Spacer(modifier = Modifier.padding(start = 8.dp))

        Row(
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("Bind this device to user?")
            Spacer(modifier = Modifier.padding(start = 8.dp))
            Switch(
                checked = bindDevice,
                onCheckedChange = { bindDevice = it }
            )
        }

        Button(onClick = {
            onSubmit(
                ProvisioningParams(
                    ssid = ssid,
                    pop = pop,
                    password = password,
                    maxPeople = maxPeople,
                    bindDevice = bindDevice,
                    deviceName = device.device.name
                )
            )
        }
        ) {
            Text(text = "Provision Now")
        }

        Spacer(modifier = Modifier.padding(16.dp))

    }
}