package com.example.doorcompanion.screens.DeviceProvisioningScreen

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

enum class ProvisioningStepResult {
    SUCCESS,
    FAIL
}

@Composable
fun ProvisioningLogItem(
    text: String,
    result: ProvisioningStepResult
) {
    val emoji = if (result == ProvisioningStepResult.FAIL) "❌" else "✔️"
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        Text(
            emoji, modifier = Modifier.size(16.dp)
        )
        Text(text)
    }

}