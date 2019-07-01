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

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <BreakoutTrustOnboardSDK.h>
#include <string>

std::string device = std::string("/dev/ttyACM1");
int baudrate       = 115200;
std::string pin    = std::string("0000");

TEST_CASE("Initialize module", "[tob]") {
  SECTION("invalid initialization with no device") {
    REQUIRE(tobInitialize(NULL, baudrate) == -1);
  }

  SECTION("invalid initialization with invalid device") {
    REQUIRE(tobInitialize("/dev/abc", baudrate) == -1);
  }

  SECTION("valid initialization") {
    REQUIRE(tobInitialize(device.c_str(), baudrate) == 0);
  }
}

TEST_CASE("Read Type A Certificate", "[available] [cert]") {
  int ret = 0;
  uint8_t cert[PEM_BUFFER_SIZE];
  int cert_size = 0;
  uint8_t pk[DER_BUFFER_SIZE];
  int pk_size = 0;

  REQUIRE(tobInitialize(device.c_str(), baudrate) == 0);

  SECTION("read certificate") {
    REQUIRE(tobExtractAvailableCertificate(cert, &cert_size, pin.c_str()) == 0);
    REQUIRE(cert_size > 0);
    REQUIRE(cert_size <= PEM_BUFFER_SIZE);
    printf("cert_size: %d\n", cert_size);
  }

  SECTION("read key") {
    REQUIRE(tobExtractAvailablePrivateKey(pk, &pk_size, pin.c_str()) == 0);
    REQUIRE(pk_size > 0);
    REQUIRE(pk_size <= DER_BUFFER_SIZE);
    printf("pk_size: %d\n", pk_size);
  }
}

int main(int argc, const char* argv[]) {
  Catch::Session session;

  using namespace Catch::clara;
  auto cli = session.cli() |
             Opt(device, "device")["-m"]["--device"]("Path to the device or pcsc:N for a PC/SC interface") |
             Opt(baudrate, "baudrate")["-g"]["--baudrate"]("Baud rate for the serial device") |
             Opt(pin, "pin")["-p"]["--pin"]("PIN code for the Trust Onboard SIM");

  // Now pass the new composite back to Catch so it uses that
  session.cli(cli);

  // Let Catch (using Clara) parse the command line
  int returnCode = session.applyCommandLine(argc, argv);
  if (returnCode != 0)  // Indicates a command line error
    return returnCode;

  return session.run();
}
