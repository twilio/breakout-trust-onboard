#!/bin/bash

kill_timer() {
  sleep $2

  is_there=$(ps -h --pid=$1)
  
  if [ -n "${is_there}" ]; then
      kill $1 && wait $1
  fi
}

build_sample() {
  cd $1
  mkdir -p build
  cd build
  cmake -DCMAKE_LIBRARY_PATH=$2 -DTRUST_ONBOARD_INCLUDE_DIRS=$3 ..
  make
}

SERVER_OUTPUT='{"hello":"mias"}'

while [[ $# -gt 0 ]]; do
  key="$1"

  case $key in
      -m|--device)
      TOB_DEVICE="$2"
      shift
      shift
      ;;
      -g|--baudrate)
      TOB_BAUDRATE="$2"
      shift
      shift
      ;;
      -p|--pin)
      TOB_PIN="$2"
      shift
      shift
      ;;
      *)
      shift
      ;;
  esac
done

if [ -z "${TOB_DEVICE}" ] || [ -z "${TOB_BAUDRATE}" ] || [ -z ${TOB_PIN} ]; then
	echo "Usage: tls_lib_test.sh --device <path to dev or pcsc:N> --baudrate <baudrate> --pin <PIN>"
	exit 1
fi

SERVER_STDOUT=$(mktemp)

CLIENT_STDOUT=$(mktemp)

SOURCE_DIR=$(realpath `dirname "${BASH_SOURCE[0]}"`/..)
CLOUD_SUPPORT_DIR="${SOURCE_DIR}/cloud-support/custom"
TOB_LIB_DIR=${SOURCE_DIR}/install_prefix/lib
TOB_INC_DIR=${SOURCE_DIR}/install_prefix/include
export LD_LIBRARY_PATH=${TOB_LIB_DIR}

CERTS_DIR=$(mktemp -d)
${CLOUD_SUPPORT_DIR}/server/gencreds.sh ${CERTS_DIR}/CA.pem ${CERTS_DIR}/server_cert.pem ${CERTS_DIR}/server_pkey.pem localhost || exit 1

${SOURCE_DIR}/cmake/bin/trust_onboard_tool --device=${TOB_DEVICE} --baudrate=${TOB_BAUDRATE} --pin=${TOB_PIN} --available-cert=${CERTS_DIR}/client_cert_available.pem --available-key=${CERTS_DIR}/client_pkey_available.pem --signing-cert=${CERTS_DIR}/client_cert_signing.pem

echo "${SOURCE_DIR}/cmake/bin/trust_onboard_tool --device=${TOB_DEVICE} --baudrate=${TOB_BAUDRATE} --pin=${TOB_PIN} --available-cert=${CERTS_DIR}/client_cert_available.pem --available-key=${CERTS_DIR}/client_pkey_available.pem --signing-cert=${CERTS_DIR}/client_cert_signing.pem"

SERVER_CERT=${CERTS_DIR}/server_cert.pem
SERVER_PKEY=${CERTS_DIR}/server_pkey.pem
SERVER_CA=${CERTS_DIR}/CA.pem
SERVER_PORT=12345

CLIENT_SIGNING_CERT=${CERTS_DIR}/client_cert_signing.pem

echo "*** Starting HTTPS server"
${CLOUD_SUPPORT_DIR}/server/server_sample.py ${SERVER_PORT} ${SERVER_CERT} ${SERVER_PKEY} ${SOURCE_DIR}/bundles/programmable-wireless.signing.pem 2>&1 >${SERVER_STDOUT} &
server_id=$!

cleanup() {
	kill ${server_id} && wait ${server_id}
	kill ${openssl_client_id} && wait ${openssl_client_id}
	kill ${mbedtls_client_id} && wait ${mbedtls_client_id}
	kill ${wolfssl_client_id} && wait ${wolfssl_client_id}
	rm -f $SERVER_OUTPUT
	rm -f $CLIENT_STDOUT
	rm -rf $CERTS_DIR
}

trap cleanup INT TERM EXIT

sleep 2

echo "*** Testing OpenSSL sample"
build_sample ${CLOUD_SUPPORT_DIR}/client-openssl ${TOB_LIB_DIR} ${TOB_INC_DIR}

# Generate OpenSSL config

pcsc_px=$(echo ${TOB_DEVICE} | cut -d':' -f1)
pcsc_dev=$(echo ${TOB_DEVICE} | cut -d':' -f2)
export OPENSSL_CONF=$(mktemp)

if [ -n "${pcsc_dev}" ] && [ "${pcsc_px}" = "pcsc" ]; then
	# PC/SC case
	cat <<EOF >${OPENSSL_CONF}
openssl_conf = openssl_init

[openssl_init]
engines = engine_section

[engine_section]
mias = mias_section

[mias_section]
engine_id = tob_mias
dynamic_path = ${SOURCE_DIR}/cmake/lib/libtrust_onboard_engine.so
PIN = ${TOB_PIN}
PCSC_IDX = ${pcsc_dev}
PCSC = 1
EOF

else 
	# serial modem case
	cat <<EOF >${OPENSSL_CONF}
openssl_conf = openssl_init

[openssl_init]
engines = engine_section

[engine_section]
mias = mias_section

[mias_section]
engine_id = tob_mias
dynamic_path = ${SOURCE_DIR}/cmake/lib/libtrust_onboard_engine.so
PIN = ${TOB_PIN}
MODEM_DEVICE = ${TOB_DEVICE}
MODEM_BAUDRATE = ${TOB_BAUDRATE}
EOF

fi

${CLOUD_SUPPORT_DIR}/client-openssl/build/client_sample https://localhost:${SERVER_PORT} ${CLIENT_SIGNING_CERT} ${SERVER_CA} 2>&1 >${CLIENT_STDOUT} &
openssl_client_id=$!
kill_timer ${openssl_client_id} 60&
kill_timer_id=$!

wait ${openssl_client_id}
kill ${kill_timer_id}

openssl_success=$(cat ${CLIENT_STDOUT} | grep ${SERVER_OUTPUT})

if [ -n "${openssl_success}" ]; then
	echo "*** OpenSSL sample test SUCCEEDED"
else
	echo "*** OpenSSL sample test FAILED"
fi

echo "*** Testing MbedTLS sample"
build_sample ${CLOUD_SUPPORT_DIR}/client-mbedtls ${TOB_LIB_DIR} ${TOB_INC_DIR}

echo '' >${CLIENT_STDOUT}
${CLOUD_SUPPORT_DIR}/client-mbedtls/build/client_sample localhost ${SERVER_PORT} / ${CLIENT_SIGNING_CERT} ${SERVER_CA} ${TOB_DEVICE} ${TOB_BAUDRATE} ${TOB_PIN} 2>&1 >${CLIENT_STDOUT} &
mbedtls_client_id=$!
kill_timer ${mbedtls_client_id} 60&
kill_timer_id=$!

wait ${mbedtls_client_id}
kill ${kill_timer_id}

mbedtls_success=$(cat ${CLIENT_STDOUT} | grep ${SERVER_OUTPUT})

if [ -n "${mbedtls_success}" ]; then
	echo "*** MbedTLS sample test SUCCEEDED"
else
	echo "*** MbedTLS sample test FAILED"
fi

echo "*** Testing WolfSSL sample"
build_sample ${CLOUD_SUPPORT_DIR}/client-wolfssl ${TOB_LIB_DIR} ${TOB_INC_DIR}

echo '' >${CLIENT_STDOUT}
${CLOUD_SUPPORT_DIR}/client-wolfssl/build/client_sample localhost ${SERVER_PORT} / ${CLIENT_SIGNING_CERT} ${SERVER_CA} ${TOB_DEVICE} ${TOB_BAUDRATE} ${TOB_PIN} 2>&1 >${CLIENT_STDOUT} &
wolfssl_client_id=$!
kill_timer ${wolfssl_client_id} 60&
kill_timer_id=$!

wait ${wolfssl_client_id}
kill ${kill_timer_id}

wolfssl_success=$(cat ${CLIENT_STDOUT} | grep "${SERVER_OUTPUT}")
if [ -n "${wolfssl_success}" ]; then
	echo "*** WolfSSL sample test SUCCEEDED"
else
	echo "*** WolfSSL sample test FAILED"
fi

if [ -n "${openssl_success}" ] && [ -n "${mbedtls_success}" ] && [ -n "${wolfssl_success}" ]; then
	echo "*** TLS library test suite SUCCEEDED"
	exit 0
else
	echo "*** TLS library test suite FAILED"
	exit 1
fi
