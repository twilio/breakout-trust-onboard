# Trust Onboard Signing Certificate and mosquitto MQTT Example

You can authenticate with mosquitto MQTT broker over TLS and Trust Onboard using either the Available or Signing certificates.

Using the Available certificates is straight forward - export the public certificate and private key and you can provide those to your MQTT client of choice.  You can also optionally use the Trust Onboard OpenSSL Engine to avoid having to handle the export of the certificate and key yourself.

Using the Trust Onboard Signing identity requires the TLS library you are using to allow deferral of singing operations to the Trust Onboard SDK.  This is what we show in this repository.

# Approach

We'll use the Paho embedded-c MQTT client library, libcurl, and the Trust Onboard OpenSSL engine in this example.

The Paho embedded-c MQTT client library allows us to implement our own IPStack which is required to have more control over how the TLS connection is initiated.  libcurl provides a nice abstraction around the OpenSSL connection initiation and socket operations we need.  Finally, the Trust Onboard OpenSSL engine handles forwarding the signing operations to the Applet embedded in your Trust Onboard enabled SIM card.

Other paths are possible here, depending on your needs.  Support for mbedtls, wolfssl and openssl are demonstrated for HTTPS connections in the cloud-support/custom directory of this repository.

# Getting Started

You'll need a mosquitto (or other cloud provider that supports MQTT+TLS+Certificate authentication) broker instance.  You can use our [mosquitto broker in Docker](https://www.twilio.com/docs/wireless/tutorials/programmable-wireless-sample-mqtt-broker-server) tutorial to stand up a local server quickly.  Follow the tutorial to its conclusion and you'll be all set for our client to connect below.

From your MQTT backend, you will need the CA and intermediates for its TLS certificate, in a PEM file, to establish server trust.  This is optional if you override the server validation portions of setting up the TLS connection but this is not recommended for security.

You will also need to provide your backend the identity of your Trust Onboard signing certificate.  Depending on your server's configuration, this may be the Certificate SID (string starting with WCxxx) or the entire common name on the certificate.  For more details, see the MQTT broker tutorial above.

# Running the Sample

You must first configure and install the Trust Onboard SDK and custom OpenSSL engine.  You can do so with the minimal required configuration as follows:

```
cd Breakout_Trust_Onboard_SDK
mkdir cmake
cd cmake
cmake -DOPENSSL_SUPPORT=ON ..
make && sudo make install
cd ..
```

Depending on your environment, you have two ways to test this.  You can use a supported cellular module, such as the Raspberry Pi HAT that comes with our Broadband IoT Developer Kit, or a smart card reader (useful primarily for testing).

There are two Trust Onboard OpenSSL engine configuration files included in this sample.  If your cellular module lives at a different serial tty port, or if you have multiple smart card readers on your system, you may need to edit the files accordingly:

- test_engine_acm.conf - used for a serial connection to a cellular module with a Trust Onboard SIM
- test_engine_pcsc.conf - used for a direct smart card reader connection with a Trust Onboard SIM

The Eclipse project's [paho.mqtt.embedded-c library](https://github.com/eclipse/paho.mqtt.embedded-c/) is included as a git submodule of this project (be sure to `git submodule init` and `git submodule update`).  It will be compiled with the sample:

```
cd cloud-support/custom/paho-openssl
mkdir cmake
cd cmake
cmake ..
make
```

To run the sample, either with the `test_engine_acm.conf` or `test_engine_pcsc.conf` configuration, supply your own MQTT hostname, port, and topic names to subscribe to below.  The sample will subscribe to the topic and print any messages it receives.  Ensure the hostname you provide below matches one of the names on the server's certificate or host validation will fail:

```
OPENSSL_CONF=../test_engine_acm.conf ./client_sample mqtt.example.org 8883 signing ca.pem test-clientid "device/#"
```

Your client will be subscribed to the wildcard topic `device/#` when you see the following output:

```
Connecting to mqtt.example.org 8883
Subscribing to device/#
(CTRL-C to end)
```

Publish a message to your server using another authorized client and messages will be relayed as follows:

```
Message 1 arrived: qos 0, retained 0, dup 0, packetid 0
Payload hello world
```

More details on how to use Paho MQTT can be found [on the project page](https://github.com/eclipse/paho.mqtt.embedded-c).

