# Trust Onboard SDK
## Contents
1. [Overview](#overview)
1. [Example reference implementation](#Example-reference-implementation)
1. [Trust Onboard Available Certificate Extraction](#Trust-Onboard-Available-Certificate-Extraction)

[Validating Requests on the Azure IoT cloud](samples/azure-iot/README.md)

## Overview

The Breakout Trust Onboard SDK offers tools and examples on how to utilize the `available` X.509 certificate available on Twilio Wireless Trust Onboard (ToB) enabled SIM cards.

We also document a reference implementation hardware and software setup to show just one of many ways to leverage Trust Onboard with your IoT project.

Twilio Trust Onboard depends on hardware and software working together in three parts:

- The SIM has two X.509 certificates provisioned to it as well as SIM applets which provide access to and services which utilize the certificates.
- A compatible cellular module provides AT commands capable of communicating with the SIM applets and reporting the results back to the calling software.
- A library on the host platform or micro controller (referred to simply as `host` in this document) that is compatible with the cellular module's AT command set and the applets loaded on the SIM.

Twilio Trust Onboard provisions two X.509 certificates on ToB SIM cards.  An `available` certificate and a `signing` certificate.  The `available` certificate has both the X.509 public certificate and its associated private key available for export and use by your applications.  The `signing` certificate only allows the X.509 public certificate to be exported.  Cryptographic signing using this `signing` certificate only ever occurs within the SIM applet itself for security.  While the `available` certificate can be used for just about any client authentication you may wish to do that uses X.509 certificates, the `signing` certificate is primarily useful when access to the ToB SIM is built into an existing security framework, such as embedtls, openssl or wolfssl.  Plugins supporting `signing` use-cases are planned but not available today.

A typical Twilio Trust Onboard flow for utilizing the `available` certificate is as follows:

- The cellular module/development board is initialized, providing one or more serial interfaces to the host.
- A library communicates with the SIMs applets using the SIMs PIN and the path of the certificate and keys to access.  This occurs using the cellular module's serial interface and supported AT commands for accessing the SIM directly.
- The library provides the PEM encoded certificate and (in the case of the `available` certificate) private key.
- Your applications make use of the public certificate and private key for authentication with your chosen backend services.

## Building Trust Onboard SDK
Trust Onboard SDK uses `cmake` and is built as [a regular cmake project](https://cmake.org/runningcmake/):

```
    cd breakout-trust-onboard
    mkdir build
    cmake ..
    make
```

By default it has one dependency, [Niels Lohmann's single-header json parser](https://github.com/nlohmann/json). On Debian-based distributions it can be installed with `apt install nlohmann-json-dev`.

Additional configuration options include:

  * `PCSC_SUPPORT` - support for PC/SC card readers. Adds dependency on `libpcsclite` (`apt install libpcsclite1` on Debian).
  * `OPENSSL_SUPPORT` - support for OpenSSL. Adds dependency on OpenSSL.
  * `MBEDTLS_SUPPORT` - support for MbedTLS. Adds depencency on MbedTLS, should be built from source (see below).
  * `BUILD_AZURE` - support for Azure IoT SDK. Depends on Twilio build of Azure SDK (see [our Azure guide](samples/azure-iot/README.md)).
  * `BUILD_TESTS` - builds test suites, depends on OpenSSL.
  * `BUILD_DOCS` - builds documentation, depends on Doxygen.
  * `NO_OS` - minimal build for a non-posix environment. No dependencies.

## Example Reference Implementation

We have found multiple cellular modules to be compatible with the AT commands and access to the SIM needed to work with Twilio ToB SIMs, but the following is a combination which we have researched and built a tutorial around: [Twilio Wireless Broadband IoT Developer Kit](https://github.com/twilio/Wireless_Broadband_IoT_Dev_Kit)

## Trust Onboard Available Certificate Extraction

In the project you build above, you should now have an executable `bin/trust_onboard_tool`.  We can use this tool to extract the Trust Onboard `available` X.509 certificate and private key.  Since the output of this CLI is two files, we recommend writing the files out to a sub-directory.  **Important:** If you are provided a different default PIN or have changed it, be sure to replace the PIN `0000`, or you may block your SIM card.  The password `mypassword` below is the passphrase that will be used to secure the P12 certificate and key bundle the `convert_certs.sh` script generates.  This assumes you are using our pre-built image or ran `make install` for the Breakout_Trust_Onboard_SDK.

    mkdir temp
    trust_onboard_tool -d /dev/ttyACM1 -p 0000 -a temp/certificate.pem -k temp/key.pem

The certificate and private key can now be used in your applications.  We also offer direct access to the Trust Onboard SDK to access the certificate and private key in your code without an intermediate file.  See the [BreakoutTrustOnboardSDK.h](include/BreakoutTrustOnboardSDK.h) header for more details.

## OpenSSL engine

When built with `SIGNING_SUPPORT` a [dynamic engine](https://github.com/openssl/openssl/blob/master/README.ENGINE) for OpenSSL is produced that uses a signing key in the MIAS applet to establish a TLS connection. The engine supports the following control commands

    * `PIN` - MIAS PIN code (normally "0000")
    * `PCSC` - binary (0/1) command setting whether the engine uses a SIM connected over PC/SC interface (1) or to a modem accessed via a serial device (0).
    * `MODEM_DEVICE` - if PCSC is 0, a path to the serial device, otherwise ignored.
    * `PCSC_IDX` - if PCSC is 1, an index in the list returned by libpcsclite's `SCardListReaders`, otherwise ignored.
    * `LOAD_CERT_CTRL` - load public certificate from engine for either the available or signing certificate.

## MbedTLS key

When built with `MBEDTLS_SUPPORT` a dynamic library is produced providing an API to let MbedTLS key use the signing key. See the [header file](include/TobMbedtls.h) for the details.

In a resource-constrained application you most likely don't want to use this library, and probably have your own way to connect to the modem/SIM. In this way you can statically link to the low-level library. The [API](include/TbMbedtlsLL.h) allows you to substitute your own implementation of the [interface to the modem](external_libs/tob_sim/common/inc/SEInterface.h). The library doesn't use dynamic memory or multithreading.

# Sample Azure IoT Registration Client Helper

`azure_dps_registerer` is a simple client that uses the Azure IoT C SDK to register a device using the Device Provisioning Service, and get back the IoTHub URI and device ID corresponding to your certificate.

The output format is JSON:

```
    {
      "status": "SUCCESS",
      "iothub_uri": "...",
      "device_id": "..."
    }
```
