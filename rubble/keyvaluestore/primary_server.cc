#include "rocksdb/db.h"
#include "kv_store_server.h"

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


int main(int argc, char** argv) {

 rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  options.rubble_mode = true;

  std::string db_path = "/mnt/sdb/archive_dbs/temp";

  std::string server_address = "0.0.0.0:50051";
  RunServer(db_path, options, server_address);
  
  return 0;
}