/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <azure_prov_client/hsm_client_data.h>

typedef struct TWILIO_TRUST_ONBOARD_HSM_CONFIG_TAG {
  const char* device_path;
  const char* sim_pin;
  int signing;
  int baudrate;
} TWILIO_TRUST_ONBOARD_HSM_CONFIG;
