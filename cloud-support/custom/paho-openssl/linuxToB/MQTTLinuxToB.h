/*
 *
 * Trust Onboard Paho Client Sample
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#if !defined(__MQTT_LINUX_TOB_)
#define __MQTT_LINUX_TOB_

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <curl/curl.h>
#include <openssl/ssl.h>

#include <iostream>

class ToBIPStack
{
  public:
    ToBIPStack()
    {

    }

    static CURLcode restrict_digest_function(CURL* curl, void* sslctx, void* parm) {
      SSL_CTX* ctx = (SSL_CTX*)sslctx;
      SSL_CTX_set1_sigalgs_list(ctx, "RSA+SHA384");

      return CURLE_OK;
    }

    int connect(const char* hostname, int port, const char* tob_cert = "signing", const char *server_ca_pem = NULL)
    {
      CURLcode res;

      char url[100];
      snprintf(url, 100, "https://%s:%d", hostname, port); // not really https, but we need a supported libcurl protocol here.  it's not relevant since we are using easy connect / CURLOPT_CONNECT_ONLY

      curl = curl_easy_init();

      curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, restrict_digest_function);

      curl_easy_setopt(curl, CURLOPT_URL, url);
      if (curl_easy_setopt(curl, CURLOPT_SSLENGINE, "tob_mias") != CURLE_OK) {
        std::cerr << "Can't set crypto engine" << std::endl;
        return -1;
      }

      if (curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L) != CURLE_OK) {
        std::cerr << "Can't set crypto engine as default" << std::endl;
        return -1;
      }

      if (curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "ENG") != CURLE_OK) {
        std::cerr << "Can't set device certificate type to ENG" << std::endl;
        return -1;
      }

      if (curl_easy_setopt(curl, CURLOPT_SSLCERT, tob_cert) != CURLE_OK) {
        std::cerr << "Can't set device certificate identifier" << std::endl;
        return -1;
      }

      if (curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "ENG") != CURLE_OK) {
        std::cerr << "Can't set device key type to ENG" << std::endl;
        return -1;
      }

      if (curl_easy_setopt(curl, CURLOPT_SSLKEY, tob_cert) != CURLE_OK) {
        std::cerr << "Can't set device key identifier" << std::endl;
        return -1;
      }

      if (server_ca_pem != NULL) {
        if (curl_easy_setopt(curl, CURLOPT_CAINFO, server_ca_pem) != CURLE_OK) {
          std::cerr << "Can't set root CA bundle file" << std::endl;
          return -1;
        }
      }

      if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK) {
        std::cerr << "Can't enable peer verification" << std::endl;
        return -1;
      }

      if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK) {
        std::cerr << "Can't enable host verification" << std::endl;
        return -1;
      }

      if (curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L) != CURLE_OK) {
        std::cerr << "Can't enable easy send / connect only mode" << std::endl;
        return -1;
      }

      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return -1;
      }

      return 0;
    }

    int read(unsigned char* buffer, int len, int timeout_ms)
    {
      size_t bytes = 0;
      CURLcode res = curl_easy_recv(curl, &buffer[bytes], len, &bytes);
      if (res != CURLE_OK && res != CURLE_AGAIN) {
        bytes = -1;
      }

      return (int)bytes;
    }

    int write(unsigned char* buffer, int len, int timeout_ms)
    {
      size_t bytes = 0;
      CURLcode res = curl_easy_send(curl, buffer, len, &bytes);
      if (res != CURLE_OK && res != CURLE_AGAIN) {
        bytes = -1;
      }

      return (int)bytes;
    }

    int disconnect()
    {
      curl_easy_cleanup(curl);

      std::cout << "Network disconnect called" << std::endl;
    }

  private:
    CURL* curl;
};

/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

class Countdown
{
public:
  Countdown()
  {

  }

  Countdown(int ms)
  {
    countdown_ms(ms);
  }


  bool expired()
  {
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&end_time, &now, &res);
    //printf("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
    //if (res.tv_sec > 0 || res.tv_usec > 0)
    //  printf("expired %d %d\n", res.tv_sec, res.tv_usec);
        return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
  }


  void countdown_ms(int ms)
  {
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {ms / 1000, (ms % 1000) * 1000};
    //printf("interval %d %d\n", interval.tv_sec, interval.tv_usec);
    timeradd(&now, &interval, &end_time);
  }


  void countdown(int seconds)
  {
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {seconds, 0};
    timeradd(&now, &interval, &end_time);
  }


  int left_ms()
  {
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&end_time, &now, &res);
    //printf("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
        return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
  }

private:

  struct timeval end_time;
};

#endif
