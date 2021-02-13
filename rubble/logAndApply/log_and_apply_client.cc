#include "log_and_apply_client.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace logAndApply;

class LogAndApplyClient {
    public: 
      LogAndApplyClient(std::shared_ptr<Channel> channel)
        : stub_(logAndApply::NewStub(channel)){}

    bool logApply(const rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>>& edit_lists){

        EditLists edit_lists_to_send;
        ConstructRequest(&edit_lists_to_send, edit_lists);

        Response reponse;
        ClientContext context;

        // The actual RPC.
        Status status = stub_->logAndApply(&context, edit_lists_to_send, &response);

        // Act upon its status.
        if (status.ok()) {
          //RPC succeeds, return whether logAndApply succeeds or not
          return response.ok();
        } else {
          std::cout << "RPC faied : " << status.error_code() << ": " << status.error_message()
                    << std::endl;
          return false;
        }
    }
      
  private:
    std::unique_ptr<logAndApply::Stub> stub_;    
}



