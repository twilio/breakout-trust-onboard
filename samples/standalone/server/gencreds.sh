#!/bin/bash

exit_usage() {
  echo "gencreds.sh /path/to/CA.pem /path/to/server_cert.pem /path/to/server_pkey.pem server_host_name" >&2
  exit 1
}

exit_error() {
  echo $1 >&2
  exit 1
}

if [ "$#" -ne 4 ]; then
  exit_usage
fi

CA_CERT_PATH="$1"
CA_PKEY_PATH=$(mktemp)
SERVER_CSR_PATH=$(mktemp)
SERVER_CERT_PATH="$2"
SERVER_PKEY_PATH="$3"
SERVER_HOSTNAME="$4"

openssl genrsa -out ${CA_PKEY_PATH} 4096 || exit_error "Can't generate CA key"
openssl req -x509 -new -nodes -key ${CA_PKEY_PATH} -sha256 -days 1825 -out ${CA_CERT_PATH} -subj "/C=IS/ST=Iceland/L=Reykjavík/O=MIAS Test/OU=Certificate issuing department/CN=example.is"

openssl genrsa -out ${SERVER_PKEY_PATH} 2048
openssl req -new -key ${SERVER_PKEY_PATH} -out ${SERVER_CSR_PATH} -subj "/C=IS/ST=Iceland/L=Reykjavik/O=MIAS Test/OU=QA Department/CN=${SERVER_HOSTNAME}"

openssl x509 -req -in ${SERVER_CSR_PATH} -CA ${CA_CERT_PATH} -CAkey ${CA_PKEY_PATH}  -out ${SERVER_CERT_PATH} -days 1825 -sha256 -CAcreateserial

rm ${CA_PKEY_PATH}
rm ${SERVER_CSR_PATH}

