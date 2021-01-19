
#include <string.h>

#include <vector>
#include <fstream>
#include <iostream>

const std::string FLUSH =  "FLUSH";
const std::string COMPACTION = "COMPACTION";

const int num_of_dirs_to_monitor = 1;

const char db_path[] =  "/mnt/sdb/archive_dbs/";

const char* compaction_meta_path = "/mnt/sdb/archive_dbs/compaction_meta";

const char* local_sst_dir = "/mnt/sdb/archive_dbs/sst_dir/sst_last_run/";
const char* remote_sst_dir = "/mnt/nvme1n1p4/archive_dbs/sst_dir/sst_last_run/";
