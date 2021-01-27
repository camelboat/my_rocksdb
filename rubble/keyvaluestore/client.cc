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
  void GetValues(const std::vector<std::string>& keys) {
    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    auto stream = stub_->GetValues(&context);
    for (const auto& key : keys) {
      // Key we are sending to the server.
      GetRequest request;
      request.set_key(key);
      stream->Write(request);

      // Get the value for the sent key
      GetReply response;
      stream->Read(&response);
      std::cout << key << " : " << response.value() << "\n";
    }

    stream->WritesDone();
    Status status = stream->Finish();
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      std::cout << "RPC failed";
    }
  }

  void Put(const std::vector<std::pair<std::string, std::string>>& kvs){
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

    stream->WriteDone();
    Status s = stream->Finish();
    if(!s.ok()){
        std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
        std::cout << "RPC failed";
    }
  }

 private:
  std::unique_ptr<KeyValueStore::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  // In this example, we are using a cache which has been added in as an
  // interceptor.
  grpc::ChannelArguments args;
//   std::vector<
//       std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>>
//       interceptor_creators;
//   interceptor_creators.push_back(std::unique_ptr<CachingInterceptorFactory>(
//       new CachingInterceptorFactory()));

  auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
      "10.10.1.2:50024", grpc::InsecureChannelCredentials(), args,
      nullptr);
  KeyValueStoreClient client(channel);
  std::vector<std::string> keys = {"key1", "key2", "key3", "key4",
                                   "key5", "key1", "key2", "key4"};
  client.GetValues(keys);

  std::vector<std::pair<std::string, std::string>> kvs;

  for(int i = 0; i < 1000; i++){
      kvs.emplace_back("key" + i, "val" + i);
  }

  client.Put(kvs);

  return 0;
}