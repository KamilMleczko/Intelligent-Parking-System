package com.example.doorcompanion

import Screen
import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.content.ContextWrapper
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.espressif.provisioning.ESPConstants.SecurityType
import com.espressif.provisioning.ESPConstants.TransportType
import com.espressif.provisioning.ESPDevice
import com.espressif.provisioning.ESPProvisionManager
import com.example.doorcompanion.routing.Router
import com.example.doorcompanion.screens.BleScanScreen.BleScanScreen
import com.example.doorcompanion.screens.DeviceProvisioningScreen.DeviceProvisioningScreen
import com.example.doorcompanion.ui.theme.DoorCompanionTheme

class MainActivity : ComponentActivity() {

    private lateinit var manager: ESPProvisionManager
    private lateinit var espDevice: ESPDevice


    @SuppressLint("UnusedMaterial3ScaffoldPaddingParameter")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Create the ESPProvisionManager instance and the ESPDevice here
        manager = ESPProvisionManager.getInstance(this)
        espDevice = manager.createESPDevice(TransportType.TRANSPORT_BLE, SecurityType.SECURITY_1)

        enableEdgeToEdge()

        setContent {
            DoorCompanionTheme {
                Scaffold(
                    modifier = Modifier
                        .fillMaxWidth()
                        .fillMaxHeight()

                ) {
                    Column(
                        modifier = Modifier
                            .padding(horizontal = 8.dp, vertical = 24.dp)
                            .fillMaxHeight()
                            .fillMaxWidth()
                    ) {

                        AppRoot()
                    }
                }
            }
        }
    }
}

/**
 * Helper function to walk up the Context tree and find an Activity.
 */
fun Context.findActivity(): Activity {
    var context = this
    while (context is ContextWrapper) {
        if (context is Activity) return context
        context = context.baseContext
    }
    throw IllegalStateException("No Activity in context chain.")
}


@Composable
fun AppRoot(
    router: Router = viewModel()
) {
    when (val currentScreen = router.currentScreen.collectAsState().value) {
        is Screen.BleScan -> {
            BleScanScreen(
                router = router,
            )
        }

        is Screen.DeviceProvisioning -> {
            DeviceProvisioningScreen(
                currentScreen.bleDevice,
                router = router,
            )
        }

        is Screen.ProvisionSuccess -> {
            
        }

        else -> {
            toastNotify(LocalContext.current, "An error occured $currentScreen")
        }
    }
    if (router.pageNumber.collectAsState().value > 1) {
        Row(
            verticalAlignment = Alignment.Bottom,
            horizontalArrangement = Arrangement.Start,
            modifier = Modifier
                .fillMaxWidth()
                .fillMaxHeight()
        ) {

            Button(
                onClick = {
                    router.goBack()
                }
            ) {
                Text("Go back")
            }
        }
    }

}

