#include <iostream>
#include <memory>
#include <string>
#include <vector>

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
  Status Get(const std::vector<std::string>& keys, std::string value) {
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    auto stream = stub_->Get(&context);
    for (const auto& key : keys) {
      // Key we are sending to the server.
      GetRequest request;
      request.set_key(key);
      stream->Write(request);

      // Get the value for the sent key
      GetReply response;
      stream->Read(&response);
      value = response.value();
      std::cout << key << " : " << response.value() << "\n";
    }

    stream->WritesDone();
    Status status = stream->Finish();
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      std::cout << "RPC failed";
    }
    return status;
  }

  Status Put(const std::vector<std::pair<std::string, std::string>>& kvs){
    ClientContext context;
    auto stream = stub_->Put(&context);
    for(const auto& kv: kvs){
        
        PutRequest request;
        request.set_key(kv.first);
        request.set_value(kv.second);

        stream->Write(request);

        PutReply reply;
        stream->Read(&reply);
        std::cout << "Put : ( " << kv.first << " ," << kv.second << " ) , status : " << reply.ok() << "\n";  
    }

    stream->WritesDone();

    Status s = stream->Finish();
    if(!s.ok()){
        std::cout << s.error_code() << ": " << s.error_message()
                << std::endl;
        std::cout << "RPC failed";
    }
    return s;
  }

 private:
  std::unique_ptr<KeyValueStore::Stub> stub_;
};

int main(int argc, char** argv) {
   // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string target_str;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target=" << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    target_str = "localhost:50051";
  }

  KeyValueStoreClient client(grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials()));


  Status s;
  std::vector<std::pair<std::string, std::string>> kvs;
  for (int i =0; i < 10; i++){
    kvs.emplace_back("key" + std::to_string(i), "val" + std::to_string(i));
  }

  s = client.Put(kvs);
  // assert(s.ok());
  
  std::vector<std::string> keys;
  for (int i =0; i < 10; i++){
    keys.emplace_back("key" + std::to_string(i));
  }

  std::string value;
  s = client.Get(keys, value);
  // assert(s.ok());
  // assert(value == "value1");
  // std::cout << "get value for key1 : " << value << "\n";

  // std::vector<std::pair<std::string, std::string>> kvs;

  // for(int i = 0; i < 1000; i++){
  //     kvs.emplace_back("key" + i, "val" + i);
  // }

  // client.Put(kvs);

  return 0;
}