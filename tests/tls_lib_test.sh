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

MQTT_BROKER_STDOUT=$(mktemp)

MOSQUITTO_CONF=$(mktemp)

SOURCE_DIR=$(realpath `dirname "${BASH_SOURCE[0]}"`/..)
SAMPLES_DIR="${SOURCE_DIR}/samples/standalone"
TOB_LIB_DIR=${SOURCE_DIR}/install_prefix/lib
TOB_INC_DIR=${SOURCE_DIR}/install_prefix/include
export LD_LIBRARY_PATH=${TOB_LIB_DIR}

CERTS_DIR=$(mktemp -d)
${SAMPLES_DIR}/server/gencreds.sh ${CERTS_DIR}/CA.pem ${CERTS_DIR}/server_cert.pem ${CERTS_DIR}/server_pkey.pem localhost || exit 1

${SOURCE_DIR}/cmake/bin/trust_onboard_tool --device=${TOB_DEVICE} --baudrate=${TOB_BAUDRATE} --pin=${TOB_PIN} --available-cert=${CERTS_DIR}/client_cert_available.pem --available-key=${CERTS_DIR}/client_pkey_available.pem --signing-cert=${CERTS_DIR}/client_cert_signing.pem

echo "${SOURCE_DIR}/cmake/bin/trust_onboard_tool --device=${TOB_DEVICE} --baudrate=${TOB_BAUDRATE} --pin=${TOB_PIN} --available-cert=${CERTS_DIR}/client_cert_available.pem --available-key=${CERTS_DIR}/client_pkey_available.pem --signing-cert=${CERTS_DIR}/client_cert_signing.pem"

SERVER_CERT=${CERTS_DIR}/server_cert.pem
SERVER_PKEY=${CERTS_DIR}/server_pkey.pem
SERVER_CA=${CERTS_DIR}/CA.pem
SERVER_PORT=12345
MQTT_BROKER_PORT=54321

CLIENT_SIGNING_CERT=${CERTS_DIR}/client_cert_signing.pem

echo "*** Starting HTTPS server"
openssl rehash ${SOURCE_DIR}/bundles/
${SAMPLES_DIR}/server/server_sample.py ${SERVER_PORT} ${SERVER_CERT} ${SERVER_PKEY} ${SOURCE_DIR}/bundles/ 2>&1 >${SERVER_STDOUT} &
server_id=$!

echo "*** Starting MQTT broker"

cat <<EOF > ${MOSQUITTO_CONF}
require_certificate true
tls_version tlsv1.2
capath ${SOURCE_DIR}/bundles/
keyfile ${SERVER_PKEY}
certfile ${SERVER_CERT}
EOF

mosquitto -p ${MQTT_BROKER_PORT} -c ${MOSQUITTO_CONF} 2>&1 >${MQTT_BROKER_STDOUT} &
mqtt_broker_id=$!

cleanup() {
	kill ${server_id} && wait ${server_id}
	kill ${mqtt_broker_id} && wait ${mqtt_broker_id}
	kill ${openssl_client_id} && wait ${openssl_client_id}
	kill ${mbedtls_client_id} && wait ${mbedtls_client_id}
	kill ${wolfssl_client_id} && wait ${wolfssl_client_id}
	kill ${paho_openssl_client_id} && wait ${paho_openssl_client_id}
	rm -f $SERVER_STDOUT
	rm -f $CLIENT_STDOUT
	rm -f $MOSQUITTO_CONF
	rm -f $MQTT_BROKER_STDOUT
	rm -rf $CERTS_DIR
}

trap cleanup INT TERM EXIT

sleep 2

echo "*** Testing OpenSSL sample"
build_sample ${SAMPLES_DIR}/client-openssl ${TOB_LIB_DIR} ${TOB_INC_DIR}

# Generate OpenSSL config

export OPENSSL_CONF=$(mktemp)

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

${SAMPLES_DIR}/client-openssl/build/client_sample https://localhost:${SERVER_PORT} signing ${SERVER_CA} 2>&1 >${CLIENT_STDOUT} &
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
build_sample ${SAMPLES_DIR}/client-mbedtls ${TOB_LIB_DIR} ${TOB_INC_DIR}

echo '' >${CLIENT_STDOUT}
${SAMPLES_DIR}/client-mbedtls/build/client_sample localhost ${SERVER_PORT} / signing ${SERVER_CA} ${TOB_DEVICE} ${TOB_BAUDRATE} ${TOB_PIN} 2>&1 >${CLIENT_STDOUT} &
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
build_sample ${SAMPLES_DIR}/client-wolfssl ${TOB_LIB_DIR} ${TOB_INC_DIR}

echo '' >${CLIENT_STDOUT}
${SAMPLES_DIR}/client-wolfssl/build/client_sample localhost ${SERVER_PORT} / signing ${SERVER_CA} ${TOB_DEVICE} ${TOB_BAUDRATE} ${TOB_PIN} 2>&1 >${CLIENT_STDOUT} &
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

echo "*** Testing Paho MQTT sample"
build_sample ${SAMPLES_DIR}/paho-openssl ${TOB_LIB_DIR} ${TOB_INC_DIR}
echo '' >${CLIENT_STDOUT}
${SAMPLES_DIR}/paho-openssl/build/client_sample localhost ${MQTT_BROKER_PORT} signing ${SERVER_CA} paho-test-client hello 2>&1 >${CLIENT_STDOUT} &
paho_openssl_client_id=$!
kill_timer ${paho_openssl_client_id} 10&
kill_timer_id=$!

wait ${paho_openssl_client_id}
kill ${kill_timer_id}

paho_openssl_success=$(cat ${CLIENT_STDOUT} | grep "Subscribing to hello");

if [ -n "${paho_openssl_success}" ]; then
	echo "*** Paho OpenSSL sample test SUCCEEDED"
else
	echo "*** Paho OpenSSL sample test FAILED"
fi

# Evaluate result of tests
if [ -n "${openssl_success}" ] && [ -n "${mbedtls_success}" ] && [ -n "${wolfssl_success}" ] && [ -n "${paho_openssl_success}" ]; then
	echo "*** TLS library test suite SUCCEEDED"
	exit 0
else
	echo "*** TLS library test suite FAILED"
	exit 1
fi
