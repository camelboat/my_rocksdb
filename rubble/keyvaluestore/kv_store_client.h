#pragma once

#include <grpcpp/grpcpp.h>

#include "keyvaluestore.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using keyvaluestore::KeyValueStore;
using keyvaluestore::GetRequest;
using keyvaluestore::GetReply;
using keyvaluestore::PutRequest;
using keyvaluestore::PutReply;

class KeyValueStoreClient {
 public:
  KeyValueStoreClient(std::shared_ptr<Channel> channel)
      : stub_(KeyValueStore::NewStub(channel)) {}

  // Requests each key in the vector and displays the key and its corresponding
  // value as a pair
  Status Get(const std::vector<std::string>& keys, std::string value) {}

  Status Put(const std::vector<std::pair<std::string, std::string>>& kvs){}

 private:
  std::unique_ptr<KeyValueStore::Stub> stub_;
};