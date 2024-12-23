package com.example.doorcompanion.screens.DeviceProvisioningScreen

import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier

@Composable
fun ProvisioningLogs(logs: List<ProvisioningLogParams>) {

    LazyColumn(
        modifier = Modifier.fillMaxWidth()
    ) {
        items(logs) { log ->
            val (text, result) = log
            ProvisioningLogItem(text, result)
        }
    }


}