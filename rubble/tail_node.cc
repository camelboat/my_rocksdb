#include "util.h"
#include "rubble_server.h"

/* tail node in the chain */
int main(int argc, char** argv) {
  std::string server_addr, target_addr;
  if (argc == 3) {
      server_addr = argv[1];
      target_addr = argv[2];
  }
  
  rocksdb::DB* db = GetDBInstance("/tmp/rubble_tail", "/mnt/sdb/archive_dbs/tail/sst_dir","", target_addr, false, false, true);

  RunServer(db, server_addr, 16);
  return 0;
}
