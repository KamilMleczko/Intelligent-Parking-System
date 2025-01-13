package com.example.doorcompanion.screens.DeviceSummary

import Device
import DeviceSummaryViewModel
import RoomReadingStatus
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.wrapContentWidth
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel

@Composable
fun DeviceSummaryScreen(
    device: Device,
    deviceSummaryViewModel: DeviceSummaryViewModel = viewModel()
) {
    // Collect the state from the ViewModel

    val roomReadingStatus = deviceSummaryViewModel.roomReading.collectAsState().value

    val deviceName = device.name

    // Start observing room data
    LaunchedEffect(deviceName) {
        deviceSummaryViewModel.observeRoomData(deviceName)
    }

    // Display UI based on the status of roomReadingStatus
    when (roomReadingStatus) {
        is RoomReadingStatus.None -> {
            Text(
                text = "No data to display.",
                style = MaterialTheme.typography.bodyLarge,
                modifier = Modifier.fillMaxWidth(),
                textAlign = TextAlign.Center
            )
        }

        is RoomReadingStatus.Loading -> {
            CircularProgressIndicator(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp)
                    .wrapContentWidth(Alignment.CenterHorizontally)
            )
        }

        is RoomReadingStatus.Success -> {
            val reading = roomReadingStatus.reading
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp),
                horizontalArrangement = Arrangement.SpaceAround,
                verticalAlignment = Alignment.CenterVertically
            ) {
                NumericDisplay(current = reading.current_people)
                if (reading.current_people > 0) {
                    CircularGauge(
                        current = reading.current_people,
                        max = device.maxPeople
                    )
                }
            }
        }

        is RoomReadingStatus.Error -> {
            val errorMessage = roomReadingStatus.message
            Text(
                text = errorMessage,
                style = MaterialTheme.typography.bodyLarge,
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp),
                color = MaterialTheme.colorScheme.error,
                textAlign = TextAlign.Center
            )
        }

        else -> {
            // impossible?
        }
    }
}