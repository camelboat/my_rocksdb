#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "rocksdb/db.h"
// #include <rocksdb/db.h>
// #include "rocksdb/options.h"

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


rocksdb::DB* OpenDB(std::string db_path){

  rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  
  rocksdb::DB *db;
  rocksdb::Status s = rocksdb::DB::Open(options, db_path, &db);
            assert(s.ok());

  return db;
}

// Logic and data behind the server's behavior.
class KeyValueStoreServiceImpl final : public KeyValueStore::Service {
  public:
   explicit KeyValueStoreServiceImpl(const std::string& db_path){
      db_ = OpenDB(db_path);
    }

    ~KeyValueStoreServiceImpl(){
      delete db_;
    };

  Status Get(ServerContext* context,
                   ServerReaderWriter<GetReply, GetRequest>* stream) override {
    GetRequest request;
    while (stream->Read(&request)) {
      GetReply response;
      std::string value;

      std::cout << "calling Get on key : " << request.key();
      rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), request.key(), &value);
      std::cout << " return value : " << value << "\n";

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

   Status Put(ServerContext* context,
             ServerReaderWriter<PutReply, PutRequest> *stream) override {

    PutRequest request;
    while(stream->Read(&request)){
        PutReply reply;
        
        std::string key = request.key();
        std::string value = request.value();
        std::cout << "caliing put : (" << key << "," << value <<")\n";  
        rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), key, value);
        if (s.ok()){
            reply.set_ok(true);
        }else{
            reply.set_ok(false);
        }

        stream->Write(reply);
     }
     return Status::OK;
   }

  private:
    rocksdb::DB* db_;
};


void RunServer(const std::string& db_path) {

  std::string server_address = "0.0.0.0:50051";
  KeyValueStoreServiceImpl service(db_path);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}



int main(int argc, char** argv) {

  std::string db_path = "/mnt/sdb/archive_dbs/temp";
  RunServer(db_path);
  
  return 0;
}