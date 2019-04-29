# Sample Azure IoT Registration Client Helper

This simple client uses the Azure IoT C SDK to register a device using the Device Provisioning Service.

The purpose of this client is to support DPS provisioning operations with the current Python Azure IoT SDK.  A near-future version of the Python Azure IoT SDK will be supported for Twilio Trust Onboard without this additional piece.

The client, when run, will read the DPS ID Scope either from a file (by default, `/home/pi/azure_id_scope.txt`) or an AZURE_ID_SCOPE environment variable if set.  It will use the Trust Onboard `available` certificate on your Twilio Trust Onboard SIM and register a device with Azure IoT Hub.  The output of this command will be a JSON encoded hash with connection information and the Trust Onboard `available` certificate and key in cleartext.

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

    bin/python_tob_helper

The expected result is something similar to the following output:

    {
      status: "SUCCESS",
      iothub_uri: "...",
      device_id: "...",
      certificate: "...",
      key: "..."
    }

### Cross-compilation
Please refer to Azure's [documentation](https://github.com/Azure/azure-iot-sdk-c/blob/master/doc/SDK_cross_compile_example.md) on cross-compilation to build Azure IoT SDK. You can use the toolchain file provided for your device.

#### Building the sample
For cross-compilation you need to set the toolchain for CMake. E.g. for Raspberry Pi with Seeed LTE Cat 1 hat:

```
    mkdir cmake
    cd cmake
    cmake -DCMAKE_TOOLCHAIN_FILE=../../../../device-support/Seeed-LTE_Cat_1_Pi_HAT/toolchain.cmake ..
    make
```
