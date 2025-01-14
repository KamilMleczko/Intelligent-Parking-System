package com.example.doorcompanion.screens.DeviceProvisioningScreen

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import com.example.doorcompanion.R

@Composable
fun AwesomeSuccess() {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .fillMaxHeight(0.8f)
    ) {
        Text("Device has been successfully provisioned!")
        Icon(
            painter = painterResource(R.drawable.awesome),
            contentDescription = "Awesome success"
        )
    }
}