#include "../credentials.hpp"
#include "../config.hpp"
#include "partial_get.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <curl/curl.h>

static constexpr char content_type[] = "application/x-compressed-tar";

static inline std::string construct_range_header(INT_TYPE start, INT_TYPE end) {
  return "Range:bytes=" + std::to_string(start) + "-" + std::to_string(end);
}

static inline std::string rfc_2822_time_now() {
  time_t current;
  char rfc_2822_now[100];
  time(&current);
  strftime(rfc_2822_now,
      sizeof(rfc_2822_now),
      "%a, %d %b %Y %T %z",
      localtime(&current));
  return rfc_2822_now;
}

static inline std::string s3_string_to_sign(const std::string& file_name, const std::string& time_now) {
  return "GET\n\n"
    + std::string(content_type) + "\n"
    + time_now + "\n"
    + "/" + std::string(s3_bucket_name) + "/" + file_name;
}

static inline std::string base64_encode(unsigned char* data, unsigned int length) {
  BIO* bio, * base64;

  base64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  BIO_set_flags(base64, BIO_FLAGS_BASE64_NO_NL);
  BIO_push(base64, bio);
  INT_TYPE new_length = ((((length << 2) / 3) + 3) & ~3);
  int ret_code = BIO_write(base64, data, length);
  BIO_flush(base64);

  char* res = (char*) malloc((new_length + 1) * sizeof(char));
  std::string ret;
  if(ret_code > 0)
    BIO_read(bio, res, new_length), ret = res;

  BIO_free(base64);
  BIO_free(bio);
  free(res);

  return ret;
}

static inline std::string s3_string_signature(const std::string& file_name, const std::string& time_now) {
  const std::string& pre = s3_string_to_sign(file_name, time_now);
  unsigned int hmac_length = 20;
  unsigned char* result = (unsigned char*) malloc(sizeof(unsigned char) * hmac_length);

  auto byte_to_hex = [](unsigned int b) { 
    unsigned int lower = b & 0xF, upper = (b >> 4) & 0xF;
    char result[3];
    result[2] = '\0';
    auto nibble_to_hex = [](unsigned int nibble) {
      return nibble < 10 ? nibble + '0' : (nibble - 10) + 'a';
    };
    result[0] = nibble_to_hex(lower);
    result[1] = nibble_to_hex(upper);
    return std::string(result);
  };

  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, aws_secret_access_key, strlen(aws_secret_access_key), EVP_sha1(), NULL);
  HMAC_Update(&ctx, (unsigned char*) pre.data(), pre.length());
  HMAC_Final(&ctx, result, &hmac_length);
  HMAC_CTX_cleanup(&ctx);

  std::string ret = base64_encode(result, hmac_length);

  free(result);

  return ret;
}

static inline std::string s3_host() {
  return std::string(s3_bucket_name) + ".s3.amazonaws.com";
}

static inline std::string s3_file_url(const std::string& file_name) {
  return "https://" + s3_host() + "/" + file_name;
}

static inline std::vector<std::string> generate_get_headers(
    const std::string& file_name, const std::string& time_now,
    INT_TYPE start, INT_TYPE end) {
  return std::vector<std::string>{
    "Host: " + s3_host(),
    "Date: " + time_now,
    "Range:bytes=" + std::to_string(start) + "-" + std::to_string(end),
    "Content-Type: " + std::string(content_type),
    "Authorization: AWS " + std::string(aws_access_key_id) + ":" + s3_string_signature(file_name, time_now)
  };
}

static size_t write_function(void* ptr, size_t size, size_t nmemb, void* w) {
  ((std::string*) w)->append((const char*) ptr, size * nmemb);
  return size * nmemb;
};

std::string partial_get(const std::string& file_name, INT_TYPE start, INT_TYPE end) {
  CURLcode* ret;
  CURL* hnd;
  struct curl_slist* slist = NULL;
  const std::string& time_now = rfc_2822_time_now();
  const std::vector<std::string>& headers = generate_get_headers(file_name, time_now, start, end);
  for(const std::string& header : headers)
    slist = curl_slist_append(slist, header.data());

  hnd = curl_easy_init();
  curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, end - start + 2);
  curl_easy_setopt(hnd, CURLOPT_URL, s3_file_url(file_name).data());
  curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist);
  curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.57.0-DEV");
  curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);

  std::string data;
  curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_function);
  curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &data);

  int ret_code = curl_easy_perform(hnd);

  curl_easy_cleanup(hnd);
  hnd = NULL;
  curl_slist_free_all(slist);
  slist = NULL;
  if(ret) return "";
  else return data;
}
