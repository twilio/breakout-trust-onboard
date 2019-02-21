# Contents
1. [Overview](#overview)
1. [Library and Tools](#Library-and-Tools)
1. [Building and running the tool](#Building-and-running-the-tool)
1. [Software Prerequisites](#Software-Prerequisites)
1. [System configuration](#System-configuration)
1. [Trust Onboard Software Setup](#Trust-Onboard-Software-Setup)
1. [Start up the Seeed LTE Cat 1 HAT](#Start-up-the-Seeed-LTE-Cat-1-HAT)
1. [PPP configuration](#PPP-configuration)

[Validating Requests on the Azure IoT cloud](cloud-support/azure-iot/README.md)

# Overview

This README will guide you through some of the setup we have performed already in the [pre-built image we offer](README.md#Getting-Started-Options).  We recommend starting with the image, but if your needs differ below you will find some helpful getting setup instructions.

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

# Software Prerequisites

We recommend using [Raspbian Stretch Lite](https://www.raspberrypi.org/downloads/raspbian/) for a headless lightweight install.  The instructions below assume a Raspbian installation but the hardware and software should be compatible with other distributions as well.  You can find information on [writing an image to SD here](https://www.raspberrypi.org/documentation/installation/installing-images/README.md).

You can also use the Seeed LTE Cat 1 Pi HAT connected directly to a workstation to run the `trust_onboard_tool` as well to get started without a full hardware build.

Once the system is up and running, we will be using the [Breakout Trust Onboard SDK](https://github.com/twilio/Breakout_Trust_Onboard_SDK) repository from github on the device.  We will also use [Twilio Wireless PPP Scripts](https://github.com/twilio/wireless-ppp-scripts) for connecting with PPP to the network for data.

# System configuration

You will need to configure a number of things upon first boot:

- By default on the Raspberry Pi 3 B+, BT serial is mapped to UART0 and the expansion header UART is mapped to UART1.  For performance and stability reasons, we want to use the hardware UART for the HAT.  If you plan on only using USB serial to communicate with the hat, you can likely skip these steps (**NOTE: TBD**) and keep BT support and the default behavior of UART0.
    - Edit `/boot/config.txt` in the editor of your choice and add the following line to the end of the file:

        dtoverlay=pi3-disable-bt

    - Remap UART:

        sudo systemctl disable hciuart

- Disable serial console by editing `/boot/cmdline.txt` and removing the section `console=serial0,115200`
- Reboot after these changes

# Trust Onboard Software Setup

- Install required packages:

        sudo apt-get install -y git screen openssl cmake

- Clone and build the latest Trust Onboard tool code

        cd ~
        git clone git@github.com:twilio/Breakout_Trust_Onboard_SDK.git
        cd Breakout_Trust_Onboard_SDK
        git checkout initial-implementation
        cmake CMakeLists.txt
        make

To install the `trust_onboard_tool` and C library globally: (Recommended)

        sudo make install

# Start up the Seeed LTE Cat 1 HAT

Initially after system startup or after a reboot, the HAT will not be in a powered-on state.  This can be done with the hardware buttons on the HAT itself but it's more convenient to do it in software so it can eventually be added to the system's startup scripts.  Even if you see the Pi's `/dev/ttyAMA0` serial interface present, it does not mean the HAT itself is ready for business.

We provide a systemd service script in the Breakout_Trust_Onboard_SDK project which will set up the board's GPIO pins for power and reset and power it on after afterwards.  You can install this as a system service or use the script directly, depending on your needs:

To install as a systemd service, auto-started on boot:

    cd device-support/Seeed-LTE_Cat_1_Pi_HAT
    sudo ./install_service.sh

Or, to run manually:

    device-support/Seeed-LTE_Cat_1_Pi_HAT/service/seed_lte_hat.py <on|off>

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

# PPP configuration

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
