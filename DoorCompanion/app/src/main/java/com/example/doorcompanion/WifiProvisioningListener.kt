package com.example.doorcompanion

import android.content.Context
import android.util.Log
import com.espressif.provisioning.ESPConstants
import com.espressif.provisioning.listeners.ProvisionListener
import java.lang.Exception

class WifiProvisioningListener(
    private val context: Context
): ProvisionListener {

    private fun notify(text: String){
        Log.i("DOOR", text)
        toastNotify(context, text);
    }

    override fun deviceProvisioningSuccess() {
        notify("success in provisioning")
    }

    override fun wifiConfigApplied() {
        notify("Applied wifi config");
    }

    override fun createSessionFailed(e: Exception?) {
        notify("Create session failed")
    }

    override fun onProvisioningFailed(e: Exception?) {
        notify("on provisioning failed")
    }

    override fun provisioningFailedFromDevice(failureReason: ESPConstants.ProvisionFailureReason?) {
        notify("provisiong failed from device")
    }

    override fun wifiConfigApplyFailed(e: Exception?) {
        notify("wifi config apply failed")
    }

    override fun wifiConfigFailed(e: Exception?) {
        notify("wifi config failed")
    }

    override fun wifiConfigSent() {
        notify("wifi config sent")
    }
}