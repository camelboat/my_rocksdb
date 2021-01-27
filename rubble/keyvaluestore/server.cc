#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/options.h"

#include <grpcpp/grpcpp.h>
#include "keyvaluestore.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
using keyvaluestore::KeyValueStore;
using keyvaluestore::GetRequest;
using keyvaluestore::GetReply;
using keyvaluestore::PutRequest;
using keyvaluestore::PutReply;


class server
{

public:
    server(const std::string& db_path, const std::string& server, int port = 50024)
    : db_path_(db_path),
      srv_(server),
      port_(port){
            OpenDB();
            RunServer(); 
        }

    ~server(
        delete db_;
    );

rocksdb::Status OpenDB(){
    DB* db;
    Options options;
    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;
    
    std::string DBPath = db_path_;
    // open DB
    Status s = DB::Open(options, DBPath, &db);
    assert(s.ok());
    db_ = &db;
}

void RunServer() {

  std::string server_address(srv_ + ":" + port);
  KeyValueStoreServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case, it corresponds to an *synchronous* service.
  builder.RegisterService(&service);

  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

// Logic and data behind the server's behavior.
class KeyValueStoreServiceImpl final : public KeyValueStore::Service {

  Status GetValues(ServerContext* context,
                   ServerReaderWriter<GetResponse, GetReply>* stream) override {
    GetRequest request;
    while (stream->Read(&request)) {
      GetReply response;

      std::string value;
      rocksdb::Status s = db->Get(ReadOptions(), request.key(), &value);
      if(s.ok()){
          // find the value for the key
        response.set_value(value);
        response.set_ok(true);
      }else {
          response.set_ok(false);
      }

      stream->Write(response);
    }
    return Status::OK;
  }

   Status Put(ServerContexxt* context,
             ServerReaderWriter<PutReply, PutRequest> *stream) override {

    PutRequest request;
    while(stream->Read(&request)){
        PutReply reply;
        
        std::string key = request.key();
        std::string value = request.value();
        rocksdb::Status s = db->Put(WriteOptions(), key, value);
        if (s.ok()){
            reply.set_ok(true);
        }else{
            reply.set_ok(false);
        }

        stream->Write(reply);
     }
     return Status::OK;
   }
};

private:
    int me;
    std::string srv_;
    DB* db_;
    std::string db_path_;
};


int main(int argc, char** argv) {

  std::string srv = "10.10.1.2";
  std::string db_path = "/mnt/sdb/archive_dbs/temp";

  server srv(db_path, srv);

  return 0;
}