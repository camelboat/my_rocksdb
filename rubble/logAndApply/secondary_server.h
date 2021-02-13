#pragma once
#include <iostream>
#include <string>

#include "rocksdb/db.h"
#include "../../db/db_impl/db_impl.h"
#include "../../db/version_set.h"
#include "../../db/version_edit.h"

#include "keyvaluestore/kv_store_server.h"

#include <grpcpp/grpcpp.h>
#include "logAndApply.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using namespace logAndApply;

class Secondary : public KeyValueStoreServiceImpl, public LogAndApply::Service {
  public:
    explicit Secondary(std::string db_path)
      :KeyValueStoreServiceImpl(db_path){}

    ~Secondary(){}

    // seondary should also accept logAndApply request sent from the primary
    Status LogAndApply(ServerContext* context, const EditLists* edit_lists_received,
                       Response* response) override {}


    void PopulateVersionEdit(const EditLists_EditList_VersionEdit& v_edit, rocksdb::VersionEdit* edit){
      
      NewFiles adds = v_edit.added();
      DeleteFiles dels = v_edit.dels();

      // add deletions to edit
      for(int i = 0; i < dels.size(); i++){
        DeleteFile del = dels.del(i);
        edit->DeleteFile(del.level(), del.file_number());
      }

      // add newFile to edit
      for(int i = 0; i < adds.new_size(); i++){
        NewFile add = adds.new(i);

        NewFile_FileMetaData meta = add.meta();
        NewFile_FileMetaData_FileDescriptor& fd = meta.fd();

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

        edit->AddFile(add.level(), fd().file_number(), 0 /* path_id shoule be 0*/,
                      fd.file_size(), 
                      smallest, largest, 
                      fd.smallest_seqno(), fd.largest_seqno(),
                      false, 
                      rocksdb::kInvalidBlobFileNumber,
                      rocksdb::kUnknownOldestAncesterTime,
                      meta.file_creation_time() == nullptr ? rocksdb::kUnknownFileCreationTime : meta.file_creation_time(),
                      meta.file_checksum() == nullptr? rocksdb::KUnknownFileCheckSum : meta.file_checksum(),
                      meta.file_checksum_func_name() == nullptr ? rocksdb::kUnknownFileChecksumFuncName : meta.file_checksum_func_name()
                      )
      }
    
    }

    voidPopulateEditList(const EditLists_EditList& e_list, rocksdb::autovector<rocksdb::VersionEdit*>& edit_list){

      for(int i = 0; i < e_list.version_edit_size(); i++){
        rocksdb::VersionEdit* version_edit;
        PopulateVersionEdit(e_list.version_edit(i), version_edit);
        edit_list.emplace_back(version_edit);
      }
    }

    void PopulateEditLists(const EditLists* edit_lists_received, rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>>& edit_lists){

      for(int i = 0; i < edit_lists_received->edit_list_size(); i++){
        rocksdb::autovector<rocksdb::VersionEdit*> edit_list; 
        void PopulateEditList(edit_lists_received->edit_list(i), edit_list);
        edit_lists.emplace_back(edit_list);
      }
    }
}

