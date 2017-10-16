
Aws::SDKOptions options;

void initialize_aws() {
  Aws::InitAPI(options);
}

Aws::S3::S3Client create_client() {
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = Aws::Region::US_WEST_2;
  Aws::S3::S3Client s3_client(clientConfig);
  return s3_client;
}

void put_object(cirrus::ObjectID id, Aws::S3::S3Client& client,
                const std::string& bucket_name) {

  std::string key_name = "CIRRUS-" + std::to_string(id);
//  Aws::S3::Model::PutObjectRequest object_request;
//  object_request.WithBucket(bucket_name).WithKey(key_name);
//  
//  // Binary files must also have the std::ios_base::bin flag or'ed in
//  auto input_data = Aws::MakeShared<Aws::FStream>("PutObjectInputStream",
//      file_name.c_str(), std::ios_base::in | std::ios_base::binary);
//  
//  object_request.SetBody(input_data);
    
    auto put_object_outcome = s3_client.putObject(bucket_name, key_name, "JOAO"); 
//  auto put_object_outcome = s3_client.PutObject(object_request);
  
  if (put_object_outcome.IsSuccess()) {
      std::cout << "Done!" << std::endl;
  }
  else {
      std::cout << "PutObject error: " <<
          put_object_outcome.GetError().GetExceptionName() << " " <<
          put_object_outcome.GetError().GetMessage() << std::endl;
  }
}

void shutdown_aws() {
  Aws::ShutdownAPI(options);
}

