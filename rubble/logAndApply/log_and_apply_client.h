#pragma once
#include <iostream>
#include <string>

#include "rocksdb/db.h"

#include <grpcpp/grpcpp.h>
#include "logAndApply.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace logAndApply;

class LogAndApplyClient {
    public: 
      LogAndApplyClient(std::shared_ptr<Channel> channel)
        : stub_(logAndApply::NewStub(channel)){}

    bool logApply(const rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>>& edit_lists){}

     // populate one version edit
    void PopulateVersionEdit(EditLists::EditList::VersionEdit* edit_to_send, const rocksdb::VersionEdit* edit){
        
        NewFiles* new_files;

        // populate the added files
        for(const auto& new_file : edit->GetNewFiles()){

            const int level = new_file.first;
            const rocksdb::FileMetaData& meta = new_file.second;

            NewFile* f = new_files->add_new();

            NewFile_FileMetaData_FileDescriptor fd;
            fd.set_file_number(meta.fd.GetNumber());
            fd.set_file_size(meta.fd.GetFileSize());

            fd.set_smallest_seqno(meta.fd.smallest_seqno);
            fd.set_largest_seqno(meta.fd.largest_seqno);

            NewFile_FileMetaData f_meta;
            f_meta.set_fd(fd);
            f_meta.set_smallest_key(*meta.smallest.rep());
            f_meta.set_largest_key(*meta.largest.rep());

            // optional fields, not sure if you need those
            f_meta.set_oldest_ancestor_time(meta.oldest_ancester_time);
            f_meta.set_file_creation_time(meta.file_creation_time);
            f_meta.set_file_checksum(meta.file_checksum);
            f_meta.set_file_checksum_func_name(meta.file_checksum_func_name);

            f->set_level(level);
            f->set_meta(f_meta);
        }

        DeleteFiles* dels;
        // populate the deleted files
        for(const auto& deleted_file : edit->GetDeletedFiles()) {
          const int level = deleted_file.first;
          const uint64_t file_number = deleted_file.second;

          DeleteFile* del = dels->add_del();
          del->set_file_number = file_number;
          del->set_level = level;
        }

        edit_to_send->set_added(*new_files);
        edit_to_send->set_dels(*dels);
    }

    void PopulateEditList(EditLists::EditList *elist_to_send, const rocksdb::autovertor<rocksdb::VersionEdit*>& edit_list){

        for(const auto& edit : edit_list){
            EditLists_EditList_VersionEdit* edit_to_send = edit_to_send->add_version_edit();
            PopulateVersionEdit(edit_to_send, edit);
        }
    }
    
    void ConstructRequest(EditLists *edit_lists_to_send, const rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>& edit_lists){

        for(const auto& edit_list : edit_lists){
          EditLists_EditList* edit_list_to_send = edit_lists_to_send->add_edit_list();    
          void PopulateEditList(edit_list_to_send, edit_list);
        }
    }  
      
  private:
    std::unique_ptr<logAndApply::Stub> stub_; 
}