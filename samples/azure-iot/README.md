# Contents
1.  [Validating Requests on Azure IoT](#Validating-Requests-on-Azure-IoT)
1.  [Custom HSM plugin for Twilio Trust Onboard Azure IoT SDK C](#azure-iot-sdk-c)

# Validating Requests on Azure IoT

In order to validate requests coming from our device we need to create `device enrollments` and register them to the Azure IoT Hub Device Provisioning Service. A  `device enrollment` is a record that contains the initial desired configuration for the device(s) as part of that enrollment, including the desired IoT hub.

Note:
- For the purposes of the initial implementation we will manually create `Individual enrollments` via the Azure DPS console for their Trust Onboard SIMs. Iteratively, this process will be automated via API and a developer would be able to create `Individual enrollments` and `push` their certificates directly from the Twilio console.
- The operations below assume a developer has already obtained their ToB Available certificate and private key.
- The node.js Azure IoT SDK is used to open a connection to the Azure IoT Hub and send data.

### Step 1: Create an Individual Enrollment

Follow the instructions [here](https://docs.microsoft.com/en-us/azure/iot-dps/how-to-manage-enrollments), the section for "An individual enrollment..."

Mechanism: X.509
Primary Certificate .pem or .cer file: <select the .cer file exported from the ToB SIM>
IoT Hub Device ID: If left blank, the subjectName (WC SID) from the certificate will be used; if specified, this Device ID will be required below instead of the WC SID

Selection of how to assign devices to the hub and which hub(s) to use for the enrollments can be left at the default or customized.

Click 'Save'

### Step 2: Perform a device Enrollment

When performing an initial enrollment using DPS, you will need three parameters in addition to the public certificate and private key file paths:

- Provisioning Host - found on the DPS Overview page (e.g., `<assigned_hub_name>.azure-devices-provisioning.net') or can use global one 'global.azure-devices-provisioning.net`
- ID Scope - found on DPS Overview page (e.g., `0neFFFFFFFF`)
- Registration ID - by default, same as the ToB certificate's SID, `WC00000000000000000000000000000000`. Stored in the subjectName field of the certificate.

The quickest way to test out DPS device enrollment is with the node.js Azure IoT SDK sample located here:
`azure-iot-sdk-node/provisioning/device/samples/register_x509.js` - you will see placeholders looking for the information above.

After running `npm install` you can run simply `node register_x509.js` to perform the registration, receiving back output similar to:

    registration succeeded
    assigned hub=<assigned_hub_name>.azure-devices.net
    deviceId=WC00000000000000000000000000000000

You can verify the enrollment by visiting the Azure IoT Hub’s IoT Devices list.  Visit the dashboard, select your IoT Hub, and select `IoT devices` under `Explorers`.

### Step 3: Using the newly registered IoT Device

In `azure-iot-sdk-node/device/samples/simple_sample_device_x509.js`, you will find an example that connects and populates simulated sensor data via MQTT.

We are primarily interested at this point in the successful connection of the client, not where the data flows.
To use our recent device creation, you will need a connection string in addition to the public certificate and private key file paths:

- Connection string: Example using the assigned hub and deviceId above: `HostName=<assigned_hub_name>.azure-devices.net;DeviceId=WC00000000000000000000000000000000;x509=true`

After running `npm install`, you can simply `node simple_sample_device_x509.js` and should see a successful device connection followed by messages as they are sent to the selected Azure IoT Hub instance.

# Azure IoT SDK C

Integrating the Twilio Trust Onboard SDK directly to the Azure IoT C SDK is supported by using a custom HSM provider, which is a part of the Trust Onboard dynamic library. You should link your application to the library, and include the respective header to use it.

    #include "TobAzureHsm.h"

## Initialize the plugin for Provisioning operations

After creating your device provisioning handle, but before calling `Prov_Device_LL_Register_Device`, add the configuration:

    TWILIO_TRUST_ONBOARD_HSM_CONFIG* config = (TWILIO_TRUST_ONBOARD_HSM_CONFIG *)malloc(sizeof(TWILIO_TRUST_ONBOARD_HSM_CONFIG));
    config->device_path = strdup("/dev/ttyACM1"); // or appropriate path
    config->baudrate = 115200; // or whatever your serial modem uses
    config->sim_pin = strdup("0000"); // or appropriate pin
    config->signing = 1; // or 0 for 'available' key
    Prov_Device_LL_SetOption(handle, PROV_HSM_CONFIG_DATA, (void*)config);

## Initialize the plugin for Device connection operations

After creating the client handle, but before performing operations, add the configuration:

    TWILIO_TRUST_ONBOARD_HSM_CONFIG* config = (TWILIO_TRUST_ONBOARD_HSM_CONFIG *)malloc(sizeof(TWILIO_TRUST_ONBOARD_HSM_CONFIG));
    config->device_path = strdup("/dev/ttyACM1"); // or appropriate path
    config->baudrate = 115200; // or whatever your serial modem uses
    config->sim_pin = strdup("0000"); // or appropriate pin
    config->signing = 1; // or 0 for 'available' key
    IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_HSM_CONFIG_DATA, (void*)config);

