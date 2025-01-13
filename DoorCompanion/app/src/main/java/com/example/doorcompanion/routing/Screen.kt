import com.example.doorcompanion.BleDevice

sealed class Screen {

    data object BleScan : Screen()

    data class DeviceProvisioning(
        val bleDevice: BleDevice
    ) : Screen()


    data object Stats : Screen()

    data object Login : Screen()

    data object Signup : Screen()

    data class DeviceSummary(
        val device: Device
    ) : Screen()


}
