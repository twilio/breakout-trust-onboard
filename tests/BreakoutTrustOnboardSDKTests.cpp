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

TEST_CASE("Read credentials in PEM and DER format", "[available] [signing] [cert] [pkey]") {
  uint8_t* signing_cert_pem;
  int signing_cert_size_pem = 0;
  uint8_t* signing_cert_der;
  int signing_cert_size_der = 0;

  REQUIRE(tobInitialize(device.c_str(), baudrate) == 0);

  SECTION("available certificate") {
    uint8_t* available_cert;
    int available_cert_size = 0;
    int size_query          = 0;
    REQUIRE(tobExtractAvailableCertificate(NULL, &size_query, pin.c_str()) == 0);
    REQUIRE(size_query > 0);
    REQUIRE(size_query <= PEM_BUFFER_SIZE);
    printf("cert_size: %d\n", size_query);

    available_cert = new uint8_t[size_query];

    REQUIRE(tobExtractAvailableCertificate(available_cert, &available_cert_size, pin.c_str()) == 0);
    REQUIRE(available_cert_size == size_query);

    delete[] available_cert;
  }

  SECTION("available private key") {
    uint8_t* available_pk_der;
    int available_pk_der_size = 0;
    uint8_t* available_pk_pem;
    int available_pk_pem_size = 0;

    int der_size_query = 0;
    int size_query     = 0;
    REQUIRE(tobExtractAvailablePrivateKey(NULL, &size_query, NULL, &der_size_query, pin.c_str()) == 0);
    REQUIRE(der_size_query <= DER_BUFFER_SIZE);
    REQUIRE(size_query <= PEM_BUFFER_SIZE);

    available_pk_pem = new uint8_t[size_query];
    available_pk_der = new uint8_t[der_size_query];

    REQUIRE(tobExtractAvailablePrivateKey(available_pk_pem, &available_pk_pem_size, available_pk_der,
                                          &available_pk_der_size, pin.c_str()) == 0);
    REQUIRE(available_pk_pem_size == size_query);
    REQUIRE(available_pk_der_size == der_size_query);

    delete[] available_pk_der;
    delete[] available_pk_pem;
  }

  SECTION("signing certificate") {
    uint8_t* signing_cert_der;
    int signing_cert_der_size = 0;
    uint8_t* signing_cert_pem;
    int signing_cert_pem_size = 0;

    int der_size_query = 0;
    int size_query     = 0;
    REQUIRE(tobExtractSigningCertificate(NULL, &size_query, NULL, &der_size_query, pin.c_str()) == 0);
    REQUIRE(der_size_query <= DER_BUFFER_SIZE);
    REQUIRE(size_query <= PEM_BUFFER_SIZE);

    signing_cert_pem = new uint8_t[size_query];
    signing_cert_der = new uint8_t[der_size_query];

    REQUIRE(tobExtractSigningCertificate(signing_cert_pem, &signing_cert_pem_size, signing_cert_der,
                                         &signing_cert_der_size, pin.c_str()) == 0);
    REQUIRE(signing_cert_pem_size == size_query);
    REQUIRE(signing_cert_der_size == der_size_query);

    delete[] signing_cert_der;
    delete[] signing_cert_pem;
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
