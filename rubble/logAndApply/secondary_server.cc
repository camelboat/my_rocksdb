#include "secondary_server.h"
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
    explicit Secondary(std::string db_path
      // KeyValueStoreServiceImpl& kv_store
      )
      :KeyValueStoreServiceImpl(db_path){}

    ~Secondary(){
      delete db_;
    }

    // seondary should also accept logAndApply request sent from the primary
    Status LogAndApply(ServerContext* context, const EditLists* edit_lists_received,
                       Response* response) override {

      rocksdb::DBImpl* impl_ = static_cast<rocksdb::DBImpl*>(db_);
      rocksdb::VersionSet* version_set = impl_->versions_;
      rocksbd::ColumnFamilyData* default_cf = version_set->GetColumnFamilySet()->GetDefault();
      const rocksdb::MutableCFOptions* cf_options = default_cf->GetLatestMutableCFOptions();

      rocksdb::autovector<rocksdb::ColumnFamilyData*> cfds;
      cfds.emplace_back(default_cf);

      rocksdb::autovector<const rocksdb::MutableCFOptions*> mutable_cf_options_list;
      mutable_cf_options_list.emplace_back(cf_options);

      rocksdb::autovector<rocksdb::autovector<rocksdb::VersionEdit*>> edit_lists;
      PopulateEditLists(edit_lists_received, edit_lists);

      rocksdb::InstrumentedMutex* mu = impl_->mutex();
      rocksdb::FSDirectory* db_directory = impl_->directories_->GetDbDir();
      
      rocksdb::Status s = impl_->versions_->LogAndApply(cfds, mutable_cf_options_list, edit_lists, mu,
                       db_directory);
      
      if(s.ok()){
        response->set_ok(true);
      }else{
        response->set_ok(false);
        std::cout << "log_and_apply failed \n";
      }
      return Status::Ok;
    }
  
}

int main(int argc, char** argv) {

  std::string db_path = "/mnt/sdb/archive_dbs/temp";
  RunServer(db_path);
  
  return 0;
}