#include "S3.h"

#define DEBUG

using namespace Aws::S3;

Aws::SDKOptions options;

void s3_initialize_aws() {
  Aws::InitAPI(options);
}

Aws::S3::S3Client s3_create_client() {
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = Aws::Region::US_WEST_2;
  clientConfig.maxConnections = 1;
  clientConfig.connectTimeoutMs = 30000;
  clientConfig.requestTimeoutMs = 60000;

  Aws::S3::S3Client s3_client(clientConfig);
  return s3_client;
}

Aws::S3::S3Client* s3_create_client_ptr() {
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = Aws::Region::US_WEST_2;
  
  // try big timeout
  clientConfig.connectTimeoutMs = 30000;
  clientConfig.requestTimeoutMs = 60000;

  return new Aws::S3::S3Client(clientConfig);
}

void s3_put_object(uint64_t id, Aws::S3::S3Client& s3_client,
                const std::string& bucket_name, const std::string& object) {
  std::string key_name = "CIRRUS" + std::to_string(id);

  Model::PutObjectRequest putObjectRequest;
#ifdef DEBUG
  std::cout << "putObjectRequest building" << std::endl;
#endif
  putObjectRequest.WithBucket(bucket_name)
      .WithKey(key_name);

  auto ss = Aws::MakeShared<Aws::StringStream>("TAG");
#ifdef DEBUG
  std::cout << "writing stringstream" << std::endl;
#endif
  *ss << object;
  ss->flush();

#ifdef DEBUG
  std::cout << "setting body" << std::endl;
#endif
  putObjectRequest.SetBody(ss);

#ifdef debug
  std::cout << "putting object" << std::endl;
#endif
  auto put_object_outcome = s3_client.PutObject(putObjectRequest);

#ifdef debug
  std::cout << "checking success" << std::endl;
#endif
  if (put_object_outcome.IsSuccess()) {
      std::cout << "Done!" << std::endl;
  } else {
      std::cout << "PutObject error: " <<
          put_object_outcome.GetError().GetExceptionName() << " " <<
          put_object_outcome.GetError().GetMessage() << std::endl;
  }
}

std::string s3_get_object(uint64_t id, Aws::S3::S3Client& s3_client,
                const std::string& bucket_name) {
  std::string key_name = "CIRRUS" + std::to_string(id);
  Aws::S3::Model::GetObjectRequest object_request;
  object_request.WithBucket(bucket_name).WithKey(key_name);

  auto get_object_outcome = s3_client.GetObject(object_request);

  if (get_object_outcome.IsSuccess()) {
    std::ostringstream ss;
    auto buf = get_object_outcome.GetResult().GetBody().rdbuf();
    ss << buf;
    return ss.str();
  } else {
    std::cout << "GetObject error: " <<
       get_object_outcome.GetError().GetExceptionName() << " " <<
       get_object_outcome.GetError().GetMessage() << std::endl;
    throw std::runtime_error("Error");
  }
}

std::ostringstream* s3_get_object_fast(uint64_t id, Aws::S3::S3Client& s3_client,
                const std::string& bucket_name) {
  std::cout << "s3_get_object_fast" << std::endl;
  std::string key_name = "CIRRUS" + std::to_string(id);
  Aws::S3::Model::GetObjectRequest object_request;
  object_request.WithBucket(bucket_name).WithKey(key_name);

  auto get_object_outcome = s3_client.GetObject(object_request);

  std::cout << "s3_get_object_fast2" << std::endl;
  if (get_object_outcome.IsSuccess()) {
    std::ostringstream* ss = new std::ostringstream;
    std::cout << "s3_get_object_fast3" << std::endl;
    auto buf = get_object_outcome.GetResult().GetBody().rdbuf();
    *ss << buf;
    std::cout << "s3_get_object_fast4" << std::endl;
    return ss;
  } else {
    std::cout << "GetObject error: " <<
       get_object_outcome.GetError().GetExceptionName() << " " <<
       get_object_outcome.GetError().GetMessage() << std::endl;
    throw std::runtime_error("Error");
  }
}

void s3_shutdown_aws() {
  Aws::ShutdownAPI(options);
}

