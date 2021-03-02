#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <vector>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <string.h>

namespace ROCKSDB_NAMESPACE {

#define BUF_SIZE 65 * 1024 * 1024
#define PORT 8080

int read_sst(uint64_t id, char *buf);

int copy_sst(uint64_t from, uint64_t to);

int ship_sst(std::vector<uint64_t> sst);
}