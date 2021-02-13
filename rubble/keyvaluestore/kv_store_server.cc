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

// Logic and data behind the server's behavior.
class KeyValueStoreServiceImpl  : public KeyValueStore::Service {
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

   rocksdb::DB* GetDB(){
     return db_;
   }

  protected:
    rocksdb::DB* db_;
};

int main(int argc, char** argv) {

  std::string db_path = "/mnt/sdb/archive_dbs/temp";
  RunServer(db_path);
  
  return 0;
}