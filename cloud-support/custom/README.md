# Samples for using Trust Onboard SIMs with custom back end

## client-openssl

Sample client using libcurl and OpenSSL with Trust Onboard engine to establish a mutually authenticated connection to a custom server.

For the client to be able to find the engine, it should be run with `OPENSSL_CONF` environment variable to point to either
  - `client/test_engine_pcsc.conf` file to interface to the SIM card via PC/SC interface or
  - `client/test_engine_acm.conf` file to interface via serial modem connected to `/dev/ACM0` device.

## client-mbedtls

Sample client using mbedtls. The client depends on the Trust Onboard library built with `MBEDTLS_SUPPORT` flag and dynamically-linked mbedtls library in default configuration.

## client-wolfssl

Sample client using wolfSSL. A few points need to be taken care of with respect to the wolfSSL library.

  * Because of how wolfSSL is designed, signing algorithms not supported by Trust Onboard hardware should be disabled on the build stage.
  * In addition Trust Onboard uses "private key callbacks" feature, it should be enabled.
  * There was a bug in wolfSSL with the callbacks implementation, it was fixed in v3.14.3, so this is the earliest wolfSSL version that can be used.

The following `configure` command line can be used to configure wolfSSL for Trust Onboard:

```
./configure --enable-pkcallback --disable-rsapss --disable-sha512
```

## server

Example of a custom server authenticating Trust Onboard signing certificates. The server itself doesn't use the engine or Trust Onboard library, it's purpose is to test the sample client.

Also contains `gencreds.sh` script to generate a temporary CA and server certificate. **This script serves solely demonstration purposes, please don't use it in production environment**

## Example usage scenario

Starting the server:

    mkdir ~/test_server_ca
    cd ~/test_server_ca
    ~/Breakout_Trust_Onboard_SDK/cloud_support/custom/server/gencreds.sh CA.pem cert.pem pkey.pem localhost

    ~/Breakout_Trust_Onboard_SDK/cloud_support/custom/server/server_sample.py 12345 ./cert.pem ./pkey.pem ~/Breakout_Trust_Onboard_SDK/bundles/programmable-wireless.signing.pem

Extracting client certificate:

    trust_onboard_tool -d pcsc:0 -p 0000 -s signing-cert.pem

Connecting to the server (from a different terminal):

    OPENSSL_CONF=~/Breakout_Trust_Onboard_SDK/cloud_support/custom/client/test_engine_pcsc.conf client_sample https://localhost:12345 ./signing-cert.pem ~/test_server_ca/CA.pem

