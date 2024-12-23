import com.example.doorcompanion.BleDevice

sealed class Screen {

    data object BleScan : Screen()

    data class DeviceProvisioning(
        val bleDevice: BleDevice
    ) : Screen()

    data object ProvisionSuccess : Screen()

    data object Stats : Screen()


}
