package com.example.doorcompanion

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice

data class BleDevice(
    val device: BluetoothDevice,
    val primaryServiceUUID: String
    ){

    @SuppressLint("MissingPermission")
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is BleDevice) return false
        return device.name == other.device.name && device.address == other.device.address
    }

    @SuppressLint("MissingPermission")
    override fun hashCode(): Int {
        return (device.name?.hashCode() ?: 0) * 31 + device.address.hashCode()
    }
}