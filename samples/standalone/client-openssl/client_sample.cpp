#include <stdio.h>

#include <curl/curl.h>
#include <openssl/ssl.h>

#include <string>
#include <iostream>

static CURLcode restrict_digest_function(CURL* curl, void* sslctx, void* parm) {
  SSL_CTX* ctx = (SSL_CTX*)sslctx;
  SSL_CTX_set1_sigalgs_list(ctx, "RSA+SHA1:RSA+SHA256:RSA+SHA384");

  return CURLE_OK;
}

static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
  data->append((char*)ptr, size * nmemb);
  return size * nmemb;
}

static void print_usage() {
  std::cerr << "tob_client https://targetdomain.tld:12345/service [\"signing\"|\"available\"] "
               "/path/to/server/rootCA.pem"
            << std::endl;
}

int main(int argc, const char** argv) {
  CURL* curl;
  CURLcode res;

  if (argc != 4) {
    print_usage();
    return 1;
  }

  const char* url        = argv[1];
  const char* identifier = argv[2];
  const char* root_ca    = argv[3];

  std::string result;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  if (std::string(identifier) == "signing") {
    // Signing key only supports a limited amount of signing algorithms
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, restrict_digest_function);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  if (curl_easy_setopt(curl, CURLOPT_SSLENGINE, "tob_mias") != CURLE_OK) {
    std::cerr << "Can't set crypto engine 'tob_mias' - is it in the OpenSSL configuration?" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L) != CURLE_OK) {
    std::cerr << "Can't set crypto engine as default" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "ENG") != CURLE_OK) {
    std::cerr << "Can't set device certificate type to ENG" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_SSLCERT, identifier) != CURLE_OK) {
    std::cerr << "Can't set device certificate identifier" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "ENG") != CURLE_OK) {
    std::cerr << "Can't set device key type to ENG" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_SSLKEY, identifier) != CURLE_OK) {
    std::cerr << "Can't set device key identifier" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_CAINFO, root_ca) != CURLE_OK) {
    std::cerr << "Can't set root CA" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK) {
    std::cerr << "Can't enable peer verification" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK) {
    std::cerr << "Can't enable peer verification" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction) != CURLE_OK) {
    std::cerr << "Can't set write function" << std::endl;
    return 1;
  }

  if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result) != CURLE_OK) {
    std::cerr << "Can't set write function data" << std::endl;
    return 1;
  }

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
  }

  std::cout << result;

  curl_easy_cleanup(curl);

  curl_global_cleanup();

  return 0;
}
