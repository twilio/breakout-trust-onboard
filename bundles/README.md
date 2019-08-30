# CA certificate bundles

There are (so far) two generations of Trust Onboard SIMs: the pre-release generation, G1, and the released generation G2.

If you ordered your Trust Onboard SIM from the Twilio website, it should be the release generation. If unsure, extract e.g. signing certificate from the SIM with

```
trust_onboard_tool -d /dev/ttyACM1 -b 115200 -p 0000 -s ./signing.cert
```

and check the issuer hash with

```
# get signing certificate's issuer hash info
openssl x509 -in ./signing.cert -noout -issuer_hash

# get signing CA intermediate issuer hash from the pre-release generation (2be02dd1)
openssl x509 -in /path/to/Breakout_Trust_Onboard_SDK/bundles/Twilio_Trust_Onboard_G1_Signing_Issuer.pem -noout -hash

# get signing CA intermediate issuer hash from the release generation (8997d620)
openssl x509 -in /path/to/Breakout_Trust_Onboard_SDK/bundles/Twilio_Trust_Onboard_G2_Signing.pem -noout -hash
```
