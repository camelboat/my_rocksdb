#include "sync_client.h"


SyncClient::SyncClient(std::shared_ptr<Channel> channel)
    :stub_(RubbleKvStoreService::NewStub(channel)){
        grpc_thread_.reset(new std::thread(std::bind(&SyncClient::AsyncCompleteRpc, this)));
        stream_ = stub_->AsyncSync(&context_, &cq_, reinterpret_cast<void*>(Type::CONNECT));
    }; 

SyncClient::~SyncClient(){
    std::cout << "Shutting down client...." << std::endl;
    cq_.Shutdown();
    grpc_thread_->join();
}

void SyncClient::Sync(const std::string& args){
    SyncRequest request;
    request.set_args(args);

    if(!ready_.load()){ 
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk,  [&](){return ready_.load();});
    }

    ready_.store(false);
    stream_->Write(request, reinterpret_cast<void*>(Type::WRITE));  
}

// read a reply back for a sync request
void SyncClient::GetSyncReply() {
    // The tag is the link between our thread (main thread) and the completion
    // queue thread. The tag allows the completion queue to fan off
    // notification handlers for the specified read/write requests as they
    // are being processed by gRPC.
    stream_->Read(&reply_, reinterpret_cast<void*>(Type::READ));
}

// Loop while listening for completed responses.
// Prints out the response from the server.
void SyncClient::AsyncCompleteRpc() {
    void* got_tag;
    bool ok = false;

    // Block until the next result is available in the completion queue "cq".
    while (cq_.Next(&got_tag, &ok)) {
    // Verify that the request was completed successfully. Note that "ok"
    // corresponds solely to the request for updates introduced by Finish().
      GPR_ASSERT(ok);

      switch (static_cast<Type>(reinterpret_cast<long>(got_tag))) {
        case Type::READ:
            if(reply_.message().compare("Succeeds") != 0){
                std::cout << "[Reply]: " << reply_.message() << std::endl;
                assert(false); 
            }
            break;
        case Type::WRITE:

            ready_.store(true);
            cv_.notify_one();
            GetSyncReply();
            break;
        case Type::CONNECT:
            std::cout << "Server connected." << std::endl;
            break;
        case Type::WRITES_DONE:
            std::cout << "writesdone sent,sleeping 5s" << std::endl;
            stream_->Finish(&finish_status_, reinterpret_cast<void*>(Type::FINISH));
            break;
        case Type::FINISH:
            std::cout << "Client finish status:" << finish_status_.error_code() << ", msg:" << finish_status_.error_message() << std::endl;
            //context_.TryCancel();
            cq_.Shutdown();
            break;
        default:
            std::cerr << "Unexpected tag " << got_tag << std::endl;
            assert(false);
        }
    }
}