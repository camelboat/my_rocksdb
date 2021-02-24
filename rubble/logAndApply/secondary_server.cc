// #include "secondary_server.h"
#include <grpcpp/grpcpp.h>
#include "logAndApply.grpc.pb.h"

#include "rocksdb/db.h"
#include "../keyvaluestore/kv_store_server.h"

#include "port/port_posix.h"
#include "port/port.h"
#include "db/version_edit.h"
#include "db/db_impl/db_impl.h"

#include <iostream>
#include <string>
#include <list>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using rocksdb::VersionEdit;
using rocksdb::DBImpl;
using rocksdb::VersionSet;

using namespace logapply;

class Secondary : public KeyValueStoreServiceImpl, public logAndApply::Service {
  public:
    explicit Secondary(const std::string& db_path
      // KeyValueStoreServiceImpl& kv_store
      )
      :KeyValueStoreServiceImpl(db_path){}

    ~Secondary(){
      delete db_;
    }

    // seondary should also accept logAndApply request sent from the primary
    Status logAndApply(ServerContext* context, const EditLists* edit_lists_received,
                       Response* response) override {

      
      rocksdb::DBImpl* impl_ = (rocksdb::DBImpl*)db_;
      rocksdb::VersionSet* version_set = impl_->TEST_GetVersionSet();

      // rocksdb::VersionSet* version_set = impl_->versions_;

      rocksdb::ColumnFamilyData* default_cf = version_set->GetColumnFamilySet()->GetDefault();
      const rocksdb::MutableCFOptions* cf_options = default_cf->GetLatestMutableCFOptions();

      rocksdb::autovector<rocksdb::ColumnFamilyData*> cfds;
      cfds.emplace_back(default_cf);

      rocksdb::autovector<const rocksdb::MutableCFOptions*> mutable_cf_options_list;
      mutable_cf_options_list.emplace_back(cf_options);

      rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>> edit_lists;
      PopulateEditLists(edit_lists_received, edit_lists);

      rocksdb::InstrumentedMutex* mu = impl_->mutex();

      rocksdb::FSDirectory* db_directory = impl_->GetDataDir(default_cf, 0);
      // rocksdb::FSDirectory* db_directory = impl_->directories_->GetDbDir();

      bool isFlush = false;

      if(isFlush){
        rocksdb::MemTableList* imms = default_cf->imm();
        rocksdb::MemTableListVersion* current = imms->current();
        std::list<rocksdb::MemTable*> memlist = current->GetMemTable();
        rocksdb::MemTable* m = memlist.back();

        memlist.remove(m);
        m->MarkFlushed();
      }
      
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

    void PopulateEditLists(const EditLists* edit_lists_received, rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>>& edit_lists){

      for(int i = 0; i < edit_lists_received->edit_list_size(); i++){
        rocksdb::autovector<rocksdb::VersionEdit*> edit_list; 
        PopulateEditList(edit_lists_received->edit_list(i), edit_list);
        edit_lists.emplace_back(edit_list);
      }
    }
};

int main(int argc, char** argv) {

  std::string db_path = "/mnt/sdb/archive_dbs/temp";
  RunServer(db_path);
  
  return 0;
}