#!/bin/bash

mkdir -p cmake
mkdir -p install_prefix
cd cmake
cmake -DPCSC_SUPPORT=ON -DOPENSSL_SUPPORT=ON -DMBEDTLS_SUPPORT=ON -DBUILD_SHARED=ON -DBUILD_TESTS=ON -DBUILD_TESTS_COVERAGE=ON -DCMAKE_INSTALL_PREFIX=../install_prefix ..
make
make install

# Allow overriding modems.csv locally to test this script
#   modems.csv.local is not used in the CI process

if [ -f ../tests/modems.csv.local ]; then
	MODEMS_CSV_FILE=../tests/modems.csv.local
else
	MODEMS_CSV_FILE=../tests/modems.csv
fi

../tests/scripts/test_runner.py ${MODEMS_CSV_FILE} bin/trust_onboard_sdk_tests bin/trust_onboard_ll_tests ../tests/tls_lib_test.sh

gcov ./tests/CMakeFiles/trust_onboard_sdk_tests.dir/BreakoutTrustOnboardSDKTests.cpp.gcno ./tests/CMakeFiles/trust_onboard_sdk_tests.dir/BreakoutTrustOnboardLLTests.cpp.gcno ./CMakeFiles/TwilioTrustOnboard.dir/src/BreakoutTrustOnboardSDK.cpp.gcno
lcov --capture --directory .. --output-file raw_main_coverage.info
lcov --remove raw_main_coverage.info '*/catch.hpp' '/usr/*' --output-file main_coverage.info
lcov --list main_coverage.info
