# Contents
1.  [Overview](#overview)
2.  [Library and Tools](#Library-and-Tools)
3.  [Building and running the tool](#Building-and-running-the-tool)
4.  [Example reference implementation](#Example-reference-implementation)
5.  [Parts](#parts)
6.  [Software Prerequisites](#Software-Prerequisites)
7.  [Connections](#Connections)
8.  [System configuration](#System-configuration)
9.  [Trust Onboard Software Setup](#Trust-Onboard-Software-Setup)
10. [Start up the Seeed LTE Cat 1 HAT](#Start-up-the-Seeed-LTE-Cat-1-HAT)
11. [Trust Onboard Available Certificate Extraction](#Trust-Onboard-Available-Certificate-Extraction)
12. [PPP configuration](#PPP-configuration)
13. [Validating Requests on the Azure IoT cloud](#Validating-Requests-on-the-Azure-IoT-cloud)

# Overview

The Breakout Trust Onboard SDK offers tools and examples on how to utilize the `available` X.509 certificate available on Twilio Wireless Trust Onboard (ToB) enabled SIM cards.

We also document a reference implementation hardware and software setup to show just one of many ways to leverage Trust Onboard with your IoT project.

Twilio Trust Onboard depends on hardware and software working together in three parts:

- The SIM has two X.509 certificates provisioned to it as well as SIM applets which provide access to and services which utilize the certificates.
- A compatible cellular module provides AT commands capable of communicating with the SIM applets and reporting the results back to the calling software.
- A library on the host platform or micro controller (referred to simply as `host` in this document) that is compatible with the cellular module's AT command set and the applets loaded on the SIM.

Twilio Trust Onboard provisions two X.509 certificates on ToB SIM cards.  An `available` certificate and a `signing` certificate.  The `available` certificate has both the X.509 public certificate and its associated private key available for export and use by your applications.  The `signing` certificate only allows the X.509 public certificate to be exported.  Cryptographic signing using this `signing` certificate only ever occurs within the SIM applet itself for security.  While the `available` certificate can be used for just about any client authentication you may wish to do that uses X.509 certificates, the `signing` certificate is primarily useful when access to the ToB SIM is built into an existing security framework, such as embedtls, openssl or wolfssl.  Plugins supporting `signing` use-cases are planned but not available today.

A typical Twilio Trust Onboard flow for utilizing the `available` certificate is as follows:

- The cellular module/development board is initialized, providing one or more serial interfaces to the host.
- A library communicates with the SIM's applets using the SIM's PIN and the path of the certificate and keys to access.  This occurs using the cellular module's serial interface and supported AT commands for accessing the SIM directly.
- The library provides the PEM encoded certificate chain and (in the case of the `available` certificate) the DER encoded private key.
- Your applications make use of the public certificate and private key for authentication with your chosen backend services.

# Library and Tools

The `src/` and `external_libs/` folders contain the library code to communicate with the Twilio ToB SIM for certificate extraction as well as signing.

The `tools/` directory has the source the command line utility `trust_onboard_tool`.  This CLI tool allows you to export the `available` certificate chain and private key from your Twilio ToB SIM for use in your applications.

The `device-support/` directory contains scripts specific to development boards and modules Twilio supports and have tested with Trust Onboard.

The `convert_certs.sh` script contains useful `openssl` command lines to convert the private key to PEM format as well as build a single P12 certificate and key bundle.

# Building and running the tool

We use CMake (3.7) for configuring the build. To generate the Makefile and build the tooling:

    cmake CMakeLists.txt
    make

This will give you a CLI utility, `bin/trust_onboard_tool`.

The utility takes 4 parameters:

    trust_onboard_tool [device] [pin] [cert outfile] [key outfile]

- *device* - This will be of the form `/dev/cu.usbmodem#####` on Mac, or `/dev/ttyACM#` or `/dev/ttyAMA0` on linux.  Checking your system's log or the `/dev` directory will help you identify which to use.
- *pin* - This is the PIN on the SIM.  **Important:** You must ensure this is correct, or you will block your SIM which will require using the PUK to unblock it.
- *cert outfile* - This is the destination filename for the X.509 public certificate chain.
- *key outfile* - This is the destination filename for the X.509 certificate's private key.  **Important:** keep this key safe!  If it is leaked, your ToB SIM must be replaced to obtain a new private key, there is no way to update a SIM after manufacture with a new certificate and key.

# Example Reference Implementation

We have found multiple cellular modules to be compatible with the AT commands and access to the SIM needed to work with Twilio ToB SIMs, but the following is a combination which we have researched and built a tutorial around.

## Parts

- [Raspberry PI 3 B+](https://www.raspberrypi.org/products/raspberry-pi-3-model-b-plus/) In addition to the processor upgrades the 3 B+ gives, due to the use of USB along with the Seeed HAT below, we recommend it specifically due to its built-in protections around back-powering and improved support for hot-plugging USB peripherals.
- [Seeed LTE Cat 1 Pi HAT USA-AT&T or Europe version](http://wiki.seeedstudio.com/LTE_Cat_1_Pi_HAT/), depending on your region.
- A suitable power supply for the PI 3B+.  We strongly recommend using a known compatible adapter, such as the [CanaKit 2.5A Raspberry Pi 3 B+ Power Supply](https://www.amazon.com/dp/B07GZZT7DN)
- [A micro-usb cable](https://www.amazon.com/gp/product/B01FA4JXN0) for connecting the HAT to the Pi (more on why we need this later)
- [Micro SD card suitable for Raspberry Pi](https://www.raspberrypi.org/documentation/installation/sd-cards.md) - we like to use at least 8GB Class 10 cards for speed and extra space.

Additionally, you will find handy access to a USB keyboard and HDMI monitor (with cable) for initial setup.  After setup, our build can be run completely headless.  If you choose to purchase a case, ensure it is one you can fit a Raspberry Pi HAT into.  Be aware that the I2C grove headers on the Seeed HAT mentioned above protrude a bit taller than some other HAT's and may require additional room.

## Software Prerequisites

We recommend using [Raspbian Stretch Lite](https://www.raspberrypi.org/downloads/raspbian/) for a headless lightweight install.  The instructions below assume a Raspbian installation but the hardware and software should be compatible with other distributions as well.  You can find information on [writing an image to SD here](https://www.raspberrypi.org/documentation/installation/installing-images/README.md).

You can also use the Seeed LTE Cat 1 Pi HAT connected directly to a workstation to run the `trust_onboard_tool` as well to get started without a full hardware build.

Once the system is up and running, we will be using the [Breakout Trust Onboard SDK](https://github.com/twilio/Breakout_Trust_Onboard_SDK) repository from github on the device.  We will also use [Twilio Wireless PPP Scripts](https://github.com/twilio/wireless-ppp-scripts) for connecting with PPP to the network for data.

## Connections

A quick aside - if you have worked with Raspberry Pi HAT's before, you may have wondered why we will need a separate USB cable to attach to the Pi to the HAT in addition to using the Pi's expansion header.  The reason for this is while the Raspberry Pi expansion header exposes a single UART to the Seeed LTE Cat 1 HAT, the u-blox LARA-R2 cellular module the HAT integrates is capable of exposing 6 discrete UARTs and does so when connected by USB.  Three of these UART's can be used for AT and data simultaneously without having to configure and support multiplexing or having to suspend operations such as a running PPP connection in order to run other AT commands.  If your usecase does not require simultaneous access to ToB and PPP, you can safely skip the USB connection but for purposes of getting started, we recommend using the USB connection.

1. Put the Twilio ToB SIM into the Seeed LTE Cat 1 HAT
1. Attach the provided LTE antennas to the Seeed LTE Cat 1 HAT
1. Attach the HAT to the Raspberry Pi 3 B+
1. Plug a USB A -> USB Micro B cable from one of the USB ports on the Raspberry Pi into the micro USB port on the HAT
1. Insert your Raspbian micro SD card
1. Attach a monitor by HDMI and keyboard for initial setup
1. Plug in your Raspberry Pi power supply

## System configuration

You will need to configure a number of things upon first boot:

- By default on the Raspberry Pi 3 B+, BT serial is mapped to UART0 and the expansion header UART is mapped to UART1.  For performance and stability reasons, we want to use the hardware UART for the HAT.  If you plan on only using USB serial to communicate with the hat, you can likely skip these steps (**NOTE: TBD**) and keep BT support and the default behavior of UART0.
    - Edit `/boot/config.txt` in the editor of your choice and add the following line to the end of the file:

        dtoverlay=pi3-disable-bt

    - Remap UART:

        sudo systemctl disable hciuart

- Disable serial console by editing `/boot/cmdline.txt` and removing the section `console=serial0,115200`
- Reboot after these changes

## Trust Onboard Software Setup

- Install required packages:

        sudo apt-get install -y git screen openssl cmake

- Clone and build the latest Trust Onboard tool code

        cd ~
        git clone git@github.com:twilio/Breakout_Trust_Onboard_SDK.git
        cd Breakout_Trust_Onboard_SDK
        git checkout initial-implementation
        cmake CMakeLists.txt
        make

# Start up the Seeed LTE Cat 1 HAT

Initially after system startup or after a reboot, the HAT will not be in a powered-on state.  This can be done with the hardware buttons on the HAT itself but it's more convenient to do it in software so it can eventually be added to the system's startup scripts.  Even if you see the Pi's `/dev/ttyAMA0` serial interface present, it does not mean the HAT itself is ready for business.

We provide a script in the Breakout_Trust_Onboard_SDK project which will set up the board's GPIO pins for power and reset and power it on after afterwards:

    device-support/Seeed-LTE_Cat_1_Pi_HAT/startup.sh

After a few seconds, we should see the expected serial ports.  Perform this command `ls -l /dev/ttyA*` and see if you have lines similar to this:

        crw-rw---- 1 root dialout 166,  0 Jan 30 16:29 /dev/ttyACM0
        crw-rw---- 1 root dialout 166,  1 Jan 30 17:35 /dev/ttyACM1
        crw-rw---- 1 root dialout 166,  2 Jan 30 16:29 /dev/ttyACM2
        crw-rw---- 1 root dialout 166,  3 Jan 30 16:29 /dev/ttyACM3
        crw-rw---- 1 root dialout 166,  4 Jan 30 16:29 /dev/ttyACM4
        crw-rw---- 1 root dialout 166,  5 Jan 30 16:29 /dev/ttyACM5
        crw-rw---- 1 root dialout 204, 64 Jan 30 16:28 /dev/ttyAMA0

In this list, the `ttyACM#` ports are the serial ports that come from the USB connection to the u-blox LARA-R2 module.  The `ttyAMA0` port is the serial port we obtain through the Raspberry Pi expansion header.

If you will be using only the expansion header, not the USB connection, you will use `/dev/ttyAMA0` for both the PPP connection below and accessing Trust Onboard.  Keep in mind that Trust Onboard operations will not work when the PPP connection is on.  **We recommend using the USB connection and multiple serial port option if possible.**

If you will be using the USB connection method (recommended), we suggest you use `/dev/ttyACM0` for the PPP connection below and `/dev/ttyACM1` for the Trust Onboard connection.  Just a note - `ttyACM0`, `ttyACM1` and `ttyACM2` are equivalent for these purposes as all three can be used for either AT commands or data.

It is worth checking at this time for permission to access the serial ports.  Raspbian's default configuration should be good here – the serial devices should automatically be owned by the `dialout` group and your `pi` user should be a member.  Please keep in mind that if you create a separate user to own your services, they must be a part of this group also if they need to access ToB directly:

        grep dialout /etc/group

Your username should appear in the `dialout` group.

## Trust Onboard Available Certificate Extraction

In the project you build above, you should now have an executable `bin/trust_onboard_tool`.  We can use this tool to extract and convert the Trust Onboard `available` X.509 certificate and private key.  Since the output of this CLI is two files (a certificate chain and a private key) and the conversion script creates a further several files, we recommend writing the files out to a sub-directory.  **Important:** Be sure to replace the PIN `0000` with the PIN provided you or you may block your SIM card.  The password `mypassword` below is the passphrase that will be used to secure the P12 certificate and key bundle the `convert_certs.sh` script generates:

    mkdir temp
    bin/trust_onboard_tool /dev/ttyACM1 0000 temp/certificate-chain.pem temp/key.der
    ./convert_certs.sh temp/certificate-chain.pem temp/key.der mypassword

You should now have the following files in the `temp/` directory:

- `certificate-chain.pem` - The public certificate chain, including roots
- `certificate.pem` - Only the leaf public certificate
- `key.pem` - The private key, encoded in PEM format
- `key.der` - The private key, encoded in DER format
- `credential.pfx` -  The certificate chain and private key in a passphrase protected P12 format

## PPP configuration

We can use `/dev/ttyACM0` to provide a PPP connection over Twilio Wireless to the internet for our Pi.  The quickest path for this is to use a sample set of PPP scripts we have published on github.

First we need to install PPP and its dependencies:

    sudo apt-get install -y ppp usb-modeswitch

Next we will get the latest PPP scripts published by Twilio and modify them for our environment:

    cd ~
    wget https://github.com/twilio/wireless-ppp-scripts/archive/master.zip
    unzip master.zip
    cd wireless-ppp-scripts-master
    sed -s -i -e 's/ttyUSB0/ttyACM0/g' peers/twilio

Next, we need to copy the PPP scripts into `/etc`:

    sudo cp chatscripts/twilio /etc/chatscripts
    sudo cp peers/twilio /etc/ppp/peers

Finally, we can bring up the ppp connection:

    sudo pon twilio

Check `ifconfig` for a `ppp0` connection to appear after a short time.

If you are connected directly to the pi on the console (not over ssh), you can confirm network connectivity by disabling the wireless if enabled:

    sudo ifconfig wlan0 down
    curl http://example.org

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
