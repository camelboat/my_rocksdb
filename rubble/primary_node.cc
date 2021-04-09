#include "util.h"
#include "rubble_server.h"

/* tail node in the chain */
int main(int argc, char** argv) {
  std::string target_addr = "";
  if (argc == 2) {
      target_addr = argv[1];
  }
  const std::string server_addr = "128.110.153.244:50051"; 
  rocksdb::DB* db = GetDBInstance("/tmp/rubble_primary", "/mnt/sdb/archive_dbs/primary/sst_dir","", target_addr, false, true, false);

  RunServer(db, server_addr, 16);
  return 0;
}
