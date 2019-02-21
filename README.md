# Contents
1. [Overview](#overview)
1. [Example reference implementation](#Example-reference-implementation)
    1. [Parts](#parts)
    1. [Connections](#Connections)
    1. [Getting Started Options](#Getting-Started-Options)
1. [Trust Onboard Available Certificate Extraction](#Trust-Onboard-Available-Certificate-Extraction)

[Validating Requests on the Azure IoT cloud](cloud-support/azure-iot/README.md)

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

# Example Reference Implementation

We have found multiple cellular modules to be compatible with the AT commands and access to the SIM needed to work with Twilio ToB SIMs, but the following is a combination which we have researched and built a tutorial around.

## Parts

- [Raspberry PI 3 B+](https://www.raspberrypi.org/products/raspberry-pi-3-model-b-plus/) In addition to the processor upgrades the 3 B+ gives, due to the use of USB along with the Seeed HAT below, we recommend it specifically due to its built-in protections around back-powering and improved support for hot-plugging USB peripherals.
- [Seeed LTE Cat 1 Pi HAT USA-AT&T or Europe version](http://wiki.seeedstudio.com/LTE_Cat_1_Pi_HAT/), depending on your region.
- A suitable power supply for the PI 3B+.  We strongly recommend using a known compatible adapter, such as the [CanaKit 2.5A Raspberry Pi 3 B+ Power Supply](https://www.amazon.com/dp/B07GZZT7DN)
- [A micro-usb cable](https://www.amazon.com/gp/product/B01FA4JXN0) for connecting the HAT to the Pi (more on why we need this later)
- [Micro SD card suitable for Raspberry Pi](https://www.raspberrypi.org/documentation/installation/sd-cards.md) - we like to use at least 8GB Class 10 cards for speed and extra space.

Additionally, you will find handy access to a USB keyboard and HDMI monitor (with cable) for initial setup.  After setup, our build can be run completely headless.  If you choose to purchase a case, ensure it is one you can fit a Raspberry Pi HAT into.  Be aware that the I2C grove headers on the Seeed HAT mentioned above protrude a bit taller than some other HAT's and may require additional room.

## Connections

A quick aside - if you have worked with Raspberry Pi HAT's before, you may have wondered why we will need a separate USB cable to attach to the Pi to the HAT in addition to using the Pi's expansion header.  The reason for this is while the Raspberry Pi expansion header exposes a single UART to the Seeed LTE Cat 1 HAT, the u-blox LARA-R2 cellular module the HAT integrates is capable of exposing 6 discrete UARTs and does so when connected by USB.  Three of these UART's can be used for AT and data simultaneously without having to configure and support multiplexing or having to suspend operations such as a running PPP connection in order to run other AT commands.  If your usecase does not require simultaneous access to ToB and PPP, you can safely skip the USB connection but for purposes of getting started, we recommend using the USB connection.

1. Put the Twilio ToB SIM into the Seeed LTE Cat 1 HAT
1. Attach the provided LTE antennas to the Seeed LTE Cat 1 HAT
1. Attach the HAT to the Raspberry Pi 3 B+
1. Plug a USB A -> USB Micro B cable from one of the USB ports on the Raspberry Pi into the micro USB port on the HAT
1. Insert your Raspbian micro SD card
1. Attach a monitor by HDMI and keyboard for initial setup
1. Plug in your Raspberry Pi power supply

## Getting Started Options

We have a Raspberry Pi image, based on Raspbian (Stretch) available which includes a fully configured environment, the Breakout Trust Onboard C SDK, and the Azure IoT for C SDK set up.  You can [download this image here](https://github.com/twilio/Breakout_Trust_Onboard_SDK/releases).

Alternately, we have included the required steps to [replicate the environment yourself](README-Setup.md).

# Trust Onboard Available Certificate Extraction

In the project you build above, you should now have an executable `bin/trust_onboard_tool`.  We can use this tool to extract and convert the Trust Onboard `available` X.509 certificate and private key.  Since the output of this CLI is two files (a certificate chain and a private key) and the conversion script creates a further several files, we recommend writing the files out to a sub-directory.  **Important:** Be sure to replace the PIN `0000` with the PIN provided you or you may block your SIM card.  The password `mypassword` below is the passphrase that will be used to secure the P12 certificate and key bundle the `convert_certs.sh` script generates.  This assumes you are using our pre-built image or ran `make install` for the Breakout_Trust_Onboard_SDK.  The `convert_certs.sh` script can be found in the Breakout_Trust_Onboard_SDK directory or in this repository.

    mkdir temp
    trust_onboard_tool /dev/ttyACM1 0000 temp/certificate-chain.pem temp/key.der
    ./convert_certs.sh temp/certificate-chain.pem temp/key.der mypassword

You should now have the following files in the `temp/` directory:

- `certificate-chain.pem` - The public certificate chain, including roots
- `certificate.pem` - Only the leaf public certificate
- `key.pem` - The private key, encoded in PEM format
- `key.der` - The private key, encoded in DER format
- `credential.pfx` -  The certificate chain and private key in a passphrase protected P12 format
