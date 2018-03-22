#include "S3.hpp"

//#define DEBUG

using namespace Aws::S3;

Aws::SDKOptions options;

void s3_initialize_aws() {
  Aws::InitAPI(options);
}

Aws::S3::S3Client s3_create_client() {
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = Aws::Region::US_WEST_2;
  clientConfig.maxConnections = 2;
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

void s3_put_object(const std::string& id, Aws::S3::S3Client& s3_client,
                const std::string& bname, const std::string& object) {
  const Aws::String& key_name = id.c_str();
  const Aws::String& bucket_name = bname.c_str();

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
  if (!put_object_outcome.IsSuccess())
    throw std::runtime_error("");
}

std::string s3_get_object(const std::string& id, Aws::S3::S3Client& s3_client,
                const std::string& bname) {
  const Aws::String& key_name = id.c_str();
  const Aws::String& bucket_name = bname.c_str();

  Aws::S3::Model::GetObjectRequest object_request;
  object_request.WithBucket(bucket_name).WithKey(key_name);

  auto get_object_outcome = s3_client.GetObject(object_request);

  if (get_object_outcome.IsSuccess()) {
    std::ostringstream ss;
    auto buf = get_object_outcome.GetResult().GetBody().rdbuf();
    ss << buf;
    return ss.str();
  } else {
    throw std::runtime_error("");
  }
}

std::ostringstream* s3_get_object_fast(const std::string& id, Aws::S3::S3Client& s3_client,
                const std::string& bname) {
  const Aws::String& key_name = id.c_str();
  const Aws::String& bucket_name = bname.c_str();
  
  Aws::S3::Model::GetObjectRequest object_request;
  object_request.WithBucket(bucket_name).WithKey(key_name);

  auto get_object_outcome = s3_client.GetObject(object_request);

  if (get_object_outcome.IsSuccess()) {
    std::ostringstream* ss = new std::ostringstream;
    *ss << get_object_outcome.GetResult().GetBody().rdbuf();
    return ss;
  } else {
    throw std::runtime_error("");
  }
}

void s3_delete_object(const std::string& id, Aws::S3::S3Client& s3_client, const std::string& bname) {
  const Aws::String& key_name = id.c_str();
  const Aws::String& bucket_name = bname.c_str();

  Aws::S3::Model::DeleteObjectRequest object_request;
  object_request.WithBucket(bucket_name).WithKey(key_name);

  auto delete_object_outcome = s3_client.DeleteObject(object_request);
  if(!delete_object_outcome.IsSuccess())
    throw std::runtime_error("");
}

void s3_shutdown_aws() {
  Aws::ShutdownAPI(options);
}

