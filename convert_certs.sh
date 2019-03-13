#!/bin/bash -e

if [ "$#" -ne 3 ]; then
  echo "Usage <certificate-chain.pem> <key.der> <output p12 passphrase>"
  exit 1
fi

CERT=$1
KEY=$2
PASSPHRASE=$3

if [ ! -f "${CERT}" ]; then
  echo "Certificate file not valid."
  exit 1
fi

if [ ! -f "${KEY}" ]; then
  echo "Key file not valid."
  exit 1
fi

CERT_DIR=`dirname "${CERT}"`
KEY_DIR=`dirname "${KEY}"`

# display certificate information
cat "${CERT}" | openssl x509 \
  -text \
  -noout \
  -inform pem

# convert cert bundle to single pem (necessary?)
openssl x509 \
  -inform pem \
  -in "${CERT}" \
  -out "${CERT_DIR}"/certificate.pem
# convert der key to pem
openssl rsa \
  -inform der \
  -in "${KEY}" \
  -outform pem \
  -out "${KEY_DIR}"/key.pem

# verify hashes match:
echo -n "Chain hash: "
openssl x509 \
  -noout \
  -modulus \
  -in "${CERT}" | openssl md5
echo -n "Certificate hash: "
openssl x509 \
  -noout \
  -modulus \
  -in "${CERT_DIR}"/certificate.pem | openssl md5
echo -n "Private key hash: "
openssl rsa \
  -noout \
  -modulus \
  -in "${KEY_DIR}"/key.pem | openssl md5

# display sha1 fingerprint
echo -n "SHA1 certificate fingerprint: "
openssl x509 \
  -in "${CERT}" \
  -fingerprint \
  -sha1 \
  -noout | sed -e's/://g' | cut -d= -f2

# FIXME: In the future, the bundle will not be shipped with the SDK -
# instead it will be shared by CDN.  Update this accordingly when that
# happens.

# build p12 from cert and key
openssl pkcs12 \
  -export \
  -out "${CERT_DIR}"/credential.pfx \
  -inkey "${KEY_DIR}"/key.pem \
  -in ${CERT} \
  -certfile bundles/programmable-wireless.available.pem \
  -password pass:"${PASSPHRASE}"
