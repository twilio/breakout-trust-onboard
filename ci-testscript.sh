#!/bin/bash

mkdir -p cmake
cd cmake
cmake -DPCSC_SUPPORT=ON -DSIGNING_SUPPORT=ON -DMBEDTLS_SUPPORT=ON -DBUILD_SHARED=ON -DBUILD_TESTS=ON ..
make

# Allow overriding modems.csv locally to test this script
#   modems.csv.local is not used in the CI process

if [ -f ../tests/modems.csv.local ]; then
	MODEMS_CSV_FILE=../tests/modems.csv.local
else
	MODEMS_CSV_FILE=../tests/modems.csv
fi

../tests/test_runner.py ${MODEMS_CSV_FILE} bin/trust_onboard_sdk_tests bin/trust_onboard_ll_tests ../tests/tls_lib_test.sh


