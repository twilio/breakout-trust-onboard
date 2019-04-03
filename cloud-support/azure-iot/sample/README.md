# Sample Azure IoT Registration Client

This simple client uses the Azure IoT C SDK to register a device using the Device Provisioning Service.

The only piece of metadata needed by the client is your DPS instance's ID Scope.  For the Critical IoT kit, this can be configured by Command (SMS) from Twilio and populated into the ~/azure_id_scope.txt text file.

## Building in an environment other than the Critical IoT Kit

### Building natively
This guide assumes you are using the Azure IoT C SDK that is compatible with Twilio Trust Onboard.  If this is the case, you can skip ahead to [Building and Running the Sample](#Building-and-running-the-sample).

If you are not, you will need to build it with the following configuration options.  If you are work on this on a different platform, please use the following steps to get a similar environment:

    git clone --recursive https://github.com/Azure/azure-iot-sdk-c.git
    cd azure-iot-sdk-c
    git checkout --track origin/hsm_custom_data hsm_custom_data
    git submodule update
    mkdir cmake
    cd cmake
    cmake -Duse_openssl:bool=ON -Duse_prov_client=ON -Dbuild_provisioning_service_client=ON -Ddont_use_uploadtoblob=ON ..
    make
    sudo make install

If you encounter this warning turned error, you can safely edit the file to comment out the unused variables.  This will be fixed before production:

    <...>/azure-iot-sdk-c/provisioning_client/samples/custom_hsm_example/custom_hsm_example.c:21:26: error: ‘REGISTRATION_NAME’ defined but not used [-Werror=unused-const-variable=]
    static const char* const REGISTRATION_NAME = "Registration Name";
    <...>/azure-iot-sdk-c/provisioning_client/samples/custom_hsm_example/custom_hsm_example.c:20:26: error: ‘SYMMETRIC_KEY’ defined but not used [-Werror=unused-const-variable=]
    static const char* const SYMMETRIC_KEY = "Symmetric Key value";

#### Building and running the sample

To build the sample using the default module device (/dev/ttyACM1) and SIM PIN (0000), do the following in this directory:

    mkdir cmake
    cd cmake
    cmake ..
    make

To run the sample, your DPS ID Scope must be set in the environment.  To do this as a one-liner, using the azure_id_scope.txt file:

    AZURE_DPS_ID_SCOPE=$(cat ~/azure_id_scope.txt) bin/hello_register 

The expected result is something similar to the following output:

    Provisioning API Version: 1.2.13
    calling register device
    Opening serial port...Found serial /dev/ttyACM1 3
    Provisioning Status: PROV_DEVICE_REG_STATUS_CONNECTED
    Provisioning Status: PROV_DEVICE_REG_STATUS_ASSIGNING
    Provisioning Status: PROV_DEVICE_REG_STATUS_ASSIGNING
    Registration Information received from service: <youriothubname>.azure-devices.net!
    registration succeeded:
      iothub_uri: <youriothubname>.azure-devices.net
      device_id: WCxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

### Cross-compilation
Please refer to Azure's [documentation](https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/SDK_cross_compile_example.md) on cross-compilation to build Azure IoT SDK. You can use the toolchain file provided for your device.

#### Building the sample
For cross-compilation you need to set the toolchain for CMake. E.g. for Raspberry Pi with Seeed LTE Cat 1 hat:

```
    mkdir cmake
    cd cmake
    cmake -DCMAKE_TOOLCHAIN_FILE=../../../../device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake ..
    make

    # optionally you can make a deb file to install on the device
    cpack
```
