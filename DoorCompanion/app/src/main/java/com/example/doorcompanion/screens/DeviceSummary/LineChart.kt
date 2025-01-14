package com.example.doorcompanion.screens.DeviceSummary

import RoomReading
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.unit.dp
import kotlin.math.max

@Composable
fun LineChart(
    readings: List<RoomReading>,
    modifier: Modifier = Modifier,
    maxPeople: Int = 30
) {
    if (readings.isEmpty()) return

    // Ensure all readings have valid data (clamp current_people to 0 if negative)
    val cleanedReadings = readings.map { it.copy(current_people = max(0, it.current_people)) }

    // Find min and max values for scaling
    val minTimestamp = cleanedReadings.minOf { it.timestamp }
    val maxTimestamp = cleanedReadings.maxOf { it.timestamp }
    val maxPeopleInData = max(cleanedReadings.maxOf { it.current_people }, maxPeople)

    Canvas(modifier = modifier) {
        val chartWidth = size.width
        val chartHeight = size.height
        val padding = 40f // Padding around the chart

        val xScale = (chartWidth - padding * 2) / (maxTimestamp - minTimestamp).toFloat()
        val yScale = (chartHeight - padding * 2) / maxPeopleInData.toFloat()

        // Draw Axes
        drawLine(
            color = Color.Black,
            start = Offset(padding, chartHeight - padding),
            end = Offset(chartWidth - padding, chartHeight - padding),
            strokeWidth = 3f
        )
        drawLine(
            color = Color.Black,
            start = Offset(padding, chartHeight - padding),
            end = Offset(padding, padding),
            strokeWidth = 3f
        )

        // Draw Gridlines
        val gridLines = 5
        for (i in 0..gridLines) {
            val y = chartHeight - padding - (i * (chartHeight - padding * 2) / gridLines)
            drawLine(
                color = Color.LightGray,
                start = Offset(padding, y),
                end = Offset(chartWidth - padding, y),
                strokeWidth = 1f
            )
            drawContext.canvas.nativeCanvas.drawText(
                "${i * maxPeopleInData / gridLines}",
                padding / 4,
                y + 10, // Adjusted to center-align with gridlines
                android.graphics.Paint().apply {
                    color = android.graphics.Color.BLACK
                    textSize = 30f
                    textAlign = android.graphics.Paint.Align.RIGHT
                }
            )
        }

        // Draw Data Points and Lines
        cleanedReadings.zipWithNext { current, next ->
            val x1 = padding + (current.timestamp - minTimestamp) * xScale
            val y1 = chartHeight - padding - (current.current_people * yScale)
            val x2 = padding + (next.timestamp - minTimestamp) * xScale
            val y2 = chartHeight - padding - (next.current_people * yScale)

            // Line between points
            drawLine(
                color = Color.Blue,
                start = Offset(x1, y1),
                end = Offset(x2, y2),
                strokeWidth = 4f
            )

            // Point for current
            drawCircle(
                color = Color.Red,
                center = Offset(x1, y1),
                radius = 6f
            )
        }

        // Add X-Axis Labels (formatted timestamps)
        val labelCount = 5 // Limit the number of labels
        val interval = (maxTimestamp - minTimestamp) / labelCount
        for (i in 0..labelCount) {
            val timestamp = minTimestamp + (i * interval)
            val x = padding + (timestamp - minTimestamp) * xScale
            val timeLabel = java.text.SimpleDateFormat("HH:mm:ss", java.util.Locale.getDefault())
                .format(java.util.Date(timestamp))
            drawContext.canvas.nativeCanvas.drawText(
                timeLabel,
                x,
                chartHeight - padding / 2,
                android.graphics.Paint().apply {
                    color = android.graphics.Color.BLACK
                    textSize = 30f
                    textAlign = android.graphics.Paint.Align.CENTER
                }
            )
        }
    }
}

@Composable
fun LineChartContainer(
    readings: List<RoomReading>,
    maxPeople: Int = 100,
    modifier: Modifier = Modifier
) {
    Box(
        modifier = modifier
            .fillMaxWidth()
            .padding(16.dp)
            .background(Color.White, RoundedCornerShape(12.dp))
            .border(1.dp, Color.Gray, RoundedCornerShape(12.dp))
            .shadow(4.dp, RoundedCornerShape(12.dp))
            .padding(16.dp)
    ) {
        LineChart(
            readings = readings,
            modifier = Modifier
                .fillMaxWidth()
                .height(300.dp),
            maxPeople = maxPeople
        )
    }
}
