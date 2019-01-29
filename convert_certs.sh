#!/bin/bash -e -x

if [ "$#" -ne 3 ]; then
  echo "Usage <certificate.p11> <key.der> <output p12 passphrase>"
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
  -inform p11

# convert p11 cert to pem
openssl x509 \
  -inform p11 \
  -in "${CERT}" \
  -out "${CERT_DIR}"/credential.pem
# convert der key to pem
openssl rsa \
  -inform der \
  -in "${KEY}" \
  -outform pem \
  -out "${KEY_DIR}"/key.pem

# verify hashes match:
openssl x509 \
  -noout \
  -modulus \
  -in "${CERT_DIR}"/credential.pem | openssl md5
openssl rsa \
  -noout \
  -modulus \
  -in "${KEY_DIR}"/key.pem | openssl md5

# build p12 from cert and key
openssl pkcs12 \
  -export \
  -out "${CERT_DIR}"/credential.pfx \
  -inkey "${KEY_DIR}"/key.pem \
  -in "${CERT_DIR}"/credential.pem \
  -certfile bundles/programmable-wireless.available.bundle \
  -password pass:"${PASSPHRASE}"
