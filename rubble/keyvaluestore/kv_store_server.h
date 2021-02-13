#pragma once

#include <iostream>
#include <string>

#include "rocksdb/db.h"

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
class KeyValueStoreServiceImpl  : public KeyValueStore::Service {
  public:
   explicit KeyValueStoreServiceImpl(const std::string& db_path){ }

    ~KeyValueStoreServiceImpl(){};

  Status Get(ServerContext* context,
                   ServerReaderWriter<GetReply, GetRequest>* stream) override {  }

   Status Put(ServerContext* context,
             ServerReaderWriter<PutReply, PutRequest> *stream) override {}

  protected:
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