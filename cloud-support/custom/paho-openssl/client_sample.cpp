/*
 *
 * Trust Onboard Paho Client Sample
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include <string>
#include <iostream>

#include <MQTTClient.h>
#include "linuxToB/MQTTLinuxToB.h"

volatile int toStop = 0;
volatile int arrivedcount = 0;

void cfinish(int sig)
{
  signal(SIGINT, NULL);
  toStop = 1;
}

static void print_usage() {
  fprintf(stderr, "client_sample <mqtt host> <mqtt port> <signing|available> <serverca.pem> <clientId> <topic>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Ensure the OPENSSL_CONF environment variable is also set for Trust Onboard OpenSSL engine configuration.\n");
}

void messageArrived(MQTT::MessageData& md)
{
  MQTT::Message &message = md.message;

  printf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n",
      ++arrivedcount, message.qos, message.retained, message.dup, message.id);
  printf("Payload %.*s\n", (int)message.payloadlen, (char*)message.payload);
  fflush(stdout);
}

int main(int argc, const char** argv) {
  const char* openssl_conf = getenv("OPENSSL_CONF");
  if (argc != 7 || openssl_conf == NULL) {
    print_usage();
    return 1;
  }

  const char* host        = argv[1];
  const int   port        = atoi(argv[2]);
  const char* cert        = argv[3];
  const char* cafile      = argv[4];
  const char* clientId    = argv[5];
  const char* topic       = argv[6];

  curl_global_init(CURL_GLOBAL_DEFAULT);

  int rc = 0;
  unsigned char buf[100];
  unsigned char readbuf[100];

  signal(SIGINT, cfinish);
  signal(SIGTERM, cfinish);

  ToBIPStack ipstack = ToBIPStack();
  MQTT::Client<ToBIPStack, Countdown, 1000> client = MQTT::Client<ToBIPStack, Countdown, 1000>(ipstack);

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.willFlag = 0;
  data.MQTTVersion = 3;
  data.clientID.cstring = (char *)clientId;

  data.keepAliveInterval = 10;
  data.cleansession = 1;
  printf("Connecting to %s %d\n", host, port);

  rc = ipstack.connect(host, port, cert, cafile);
  if (rc != 0) {
    fprintf(stderr, "Failed to connect socket: %d\n", rc);
    return 1;
  }

  rc = client.connect(data);
  if (rc != 0) {
    fprintf(stderr, "Failed to connect mqtt: %d\n", rc);
    return 1;
  }

  printf("Subscribing to %s\n", topic);
  rc = client.subscribe(topic, MQTT::QOS0, messageArrived);
  if (rc == -1) {
    fprintf(stderr, "Failed to subscribe to topic: %d\n", rc);
    return 1;
  }

  printf("(CTRL-C to end)\n");
  while (!toStop)
  {
    client.yield(1000);
    usleep(500);
  }

  printf("Stopping\n");

  rc = client.disconnect();
  ipstack.disconnect();

  curl_global_cleanup();

  return 0;
}
