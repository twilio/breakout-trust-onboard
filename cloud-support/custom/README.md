# Samples for using Trust Onboard SIMs with custom back end

## client

Sample client using libcurl and OpenSSL with Trust Onboard engine to establish a mutually authenticated connection to a custom server.

For the client to be able to find the engine, it should be run with `OPENSSL_CONF` environment variable to point to either
  - `client/test_engine_pcsc.conf` file to interface to the SIM card via PC/SC interface or
  - `client/test_engine_acm.conf` file to interface via serial modem connected to `/dev/ACM0` device.

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

    trust_onboard_tool pcsc:0 0000 /dev/null /dev/null signing-cert.pem

Connecting to the server (from a different terminal):

    OPENSSL_CONF=~/Breakout_Trust_Onboard_SDK/cloud_support/custom/client/test_engine_pcsc.conf client_sample https://localhost:12345 ./signing-cert.pem ~/test_server_ca/CA.pem

