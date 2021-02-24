// #include "secondary_server.h"
// #include <grpcpp/grpcpp.h>
#include "logAndApply.grpc.pb.h"

// #include "rocksdb/db.h"
#include "port/port_posix.h"
#include "port/port.h"
#include "db/version_edit.h"
#include "db/db_impl/db_impl.h"

#include "../keyvaluestore/primary_server.h"

// #include <string>
// #include <iostream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using namespace logapply;

class Secondary : public KeyValueStoreServiceImpl, public logAndApply::Service {
  public:
    explicit Secondary(rocksdb::DB* db
      // KeyValueStoreServiceImpl& kv_store
      )
      :KeyValueStoreServiceImpl(db){};

    ~Secondary(){};

    // seondary should also accept logAndApply request sent from the primary
    Status logAndApply(ServerContext* context, const EditLists* edit_lists_received,
                       Response* response) override {
            
      rocksdb::DBImpl* impl_ = (rocksdb::DBImpl*)db_;
      rocksdb::VersionSet* version_set = impl_->TEST_GetVersionSet();
      // rocksdb::VersionSet* version_set = impl_->versions_.get();
      rocksdb::ColumnFamilyData* default_cf = version_set->GetColumnFamilySet()->GetDefault();
      const rocksdb::MutableCFOptions* cf_options = default_cf->GetLatestMutableCFOptions();

      rocksdb::autovector<rocksdb::ColumnFamilyData*> cfds;
      cfds.emplace_back(default_cf);

      rocksdb::autovector<const rocksdb::MutableCFOptions*> mutable_cf_options_list;
      mutable_cf_options_list.emplace_back(cf_options);

      rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>> edit_lists;
      PopulateEditLists(*edit_lists_received, edit_lists);

      rocksdb::InstrumentedMutex* mu = impl_->mutex();
      rocksdb::FSDirectory* db_directory = impl_->directories_.GetDbDir();
      
      rocksdb::Status s = version_set->LogAndApply(cfds, mutable_cf_options_list, edit_lists, mu,
                       db_directory);
      
      if(s.ok()){
        response->set_ok(true);
      }else{
        response->set_ok(false);
        std::cout << "log_and_apply failed \n";
      }
      return Status::OK;
    }

    void PopulateVersionEdit(const EditLists_EditList_VersionEdit& v_edit, rocksdb::VersionEdit* edit){
      
      NewFiles adds = v_edit.added();
      DeleteFiles dels = v_edit.dels();

      // add deletions to edit
      for(int i = 0; i < dels.del_size(); i++){
        DeleteFile del = dels.del(i);
        edit->DeleteFile(del.level(), del.file_number());
      }

      // add newFile to edit
      for(int i = 0; i < adds.new__size(); i++){
        NewFile add = adds.new_(i);

        NewFile_FileMetaData meta = add.meta();
        NewFile_FileMetaData_FileDescriptor fd = meta.fd();

        rocksdb::InternalKey smallest;
        std::string rep = meta.smallest_key();
        rocksdb::ParsedInternalKey parsed;
        if (rocksdb::ParseInternalKey(rocksdb::Slice(rep), &parsed) == rocksdb::Status::OK()){
            smallest.SetFrom(parsed);
        }

        rocksdb::InternalKey largest;
        rep = meta.largest_key();
        if(rocksdb::ParseInternalKey(rocksdb::Slice(rep),  &parsed) == rocksdb::Status::OK()){
          largest.SetFrom(parsed);
        }


        edit->AddFile(add.level(), fd.file_number(), 0 /* path_id shoule be 0*/,
                      fd.file_size(), 
                      smallest, largest, 
                      fd.smallest_seqno(), fd.largest_seqno(),
                      false, 
                      rocksdb::kInvalidBlobFileNumber,
                      rocksdb::kUnknownOldestAncesterTime,
                      meta.file_creation_time(),
                      meta.file_checksum(),
                      meta.file_checksum_func_name()
                      );
      }
    
    }

    void PopulateEditList(const EditLists_EditList& e_list, rocksdb::autovector<rocksdb::VersionEdit*>& edit_list){

      for(int i = 0; i < e_list.version_edit_size(); i++){
        rocksdb::VersionEdit* version_edit;
        PopulateVersionEdit(e_list.version_edit(i), version_edit);
        edit_list.emplace_back(version_edit);
      }
    }

    void PopulateEditLists(const EditLists& edit_lists_received, rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>>& edit_lists){

      for(int i = 0; i < edit_lists_received.edit_list_size(); i++){
        rocksdb::autovector<rocksdb::VersionEdit*> edit_list; 
        PopulateEditList(edit_lists_received.edit_list(i), edit_list);
        edit_lists.emplace_back(edit_list);
      }
    }
  
};

int main(int argc, char** argv) {


  rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  options.rubble_mode = true;

  std::string server_address = "0.0.0.0:50051";
  std::string db_path = "/mnt/sdb/archive_dbs/temp";

  rocksdb::DB* db = OpenDB(db_path, options);

  KeyValueStoreServiceImpl service(db);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();

  // RunServer(db_path, options, server_address);
  
  return 0;
}