# CA certificate bundles

There are (so far) two generations of Trust Onboard SIMs: the pre-release generation, G1, and the released generation G2.

If you ordered your Trust Onboard SIM from Twilio website it should be the release generation. If unsure, extract e.g. signing certificate from the SIM with

```
trust_onboard_tool -d /dev/ttyACM1 -b 115200 -p 0000 -s ./signing.cert
```

and check the issuer with

```
# get signing certificate info
openssl x509 -in ./signing.cert -text -noout

# get signing CA chain info from the pre-release generation
openssl x509 -in /path/to/Breakout_Trust_Onboard_SDK/bundles/G1/programmable-wireless.signing.pem -text -noout

# get signing CA info from the release generation
openssl x509 -in /path/to/Breakout_Trust_Onboard_SDK/bundles/G2/Twilio_Trust_Onboard_G2_Signing.pem -text -noout
```
