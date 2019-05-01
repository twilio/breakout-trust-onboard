/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "catch.hpp"

#include <BreakoutTrustOnboardSDK.h>

const char* device = "/dev/ttyACM1";
//const char* device = "/dev/cu.usbmodem14403";
const char* pin = "0000";

TEST_CASE( "Initialize module", "[tob]" ) {

  SECTION( "invalid initialization with no device" ) {
    REQUIRE ( tobInitialize(NULL) == -1 );
  }

  SECTION( "invalid initialization with invalid device" ) {
    REQUIRE ( tobInitialize("/dev/abc") == -1 );
  }

  SECTION( "valid initialization" ) {
    REQUIRE ( tobInitialize(device) == 0 );
  }
}

TEST_CASE( "Read Type A Certificate", "[available] [cert]") {

  int ret = 0;
  uint8_t cert[PEM_BUFFER_SIZE];
  int cert_size = 0;
  uint8_t pk[DER_BUFFER_SIZE];
  int pk_size = 0;

  REQUIRE ( tobInitialize(device) == 0 );

  SECTION( "read certificate" ) {
    REQUIRE ( tobExtractAvailableCertificate(cert, &cert_size, pin) == 0 );
    REQUIRE ( cert_size > 0 );
    REQUIRE ( cert_size <= PEM_BUFFER_SIZE );
    printf("cert_size: %d\n", cert_size);
  }

  SECTION( "read key" ) {
    REQUIRE ( tobExtractAvailablePrivateKey(pk, &pk_size, pin) == 0 );
    REQUIRE ( pk_size > 0 );
    REQUIRE ( pk_size <= DER_BUFFER_SIZE );
    printf("pk_size: %d\n", pk_size);
  }

}
