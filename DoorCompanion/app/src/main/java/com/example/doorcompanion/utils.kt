package com.example.doorcompanion

import android.content.Context
import android.widget.Toast
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

fun toastNotify(context: Context, text: String) {
    // Toasts are assumed to run in the main (UI) thread.
    // Calling Toast.show from a background thread may result in a crash.
    // And this is a real problem e.g., when calling this method from an event handler.
    val mainHandler = android.os.Handler(android.os.Looper.getMainLooper())
    mainHandler.post {
        val toast = Toast(context)
        toast.setText(text)
        toast.duration = Toast.LENGTH_SHORT
        toast.show()
    }
}


@Composable
fun LoadingSpinner(
    message: String = "Loading…"
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Transparent),
        contentAlignment = Alignment.Center
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            CircularProgressIndicator()
            Spacer(Modifier.height(8.dp))
            Text(text = message)
        }
    }
}