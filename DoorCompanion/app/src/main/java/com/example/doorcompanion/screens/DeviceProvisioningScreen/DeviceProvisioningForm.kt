package com.example.doorcompanion.screens.DeviceProvisioningScreen

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

data class ProvisioningParams(
    val pop: String,
    val ssid: String,
    val password: String,
)

@Composable
fun DeviceProvisioningForm(
    onSubmit: (ProvisioningParams) -> Unit
) {
    var pop by remember { mutableStateOf("abcd1234") }
    var ssid by remember { mutableStateOf("Mockinixinternet") }
    var password by remember { mutableStateOf("NienienieDlaPanaTo") }
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

        Spacer(modifier = Modifier.padding(16.dp))
        Button(onClick = {
            onSubmit(
                ProvisioningParams(
                    ssid = ssid,
                    pop = pop,
                    password = password,
                )
            )
        }
        ) {
            Text(text = "Provision Now")
        }

        Spacer(modifier = Modifier.padding(16.dp))

    }
}