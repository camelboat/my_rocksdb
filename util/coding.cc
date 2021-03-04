//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/coding.h"

#include <algorithm>
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"

#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <vector>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <string.h>

namespace ROCKSDB_NAMESPACE {

// conversion' conversion from 'type1' to 'type2', possible loss of data
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
char* EncodeVarint32(char* dst, uint32_t v) {
  // Operate on characters as unsigneds
  unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);
  static const int B = 128;
  if (v < (1 << 7)) {
    *(ptr++) = v;
  } else if (v < (1 << 14)) {
    *(ptr++) = v | B;
    *(ptr++) = v >> 7;
  } else if (v < (1 << 21)) {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = v >> 14;
  } else if (v < (1 << 28)) {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = (v >> 14) | B;
    *(ptr++) = v >> 21;
  } else {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = (v >> 14) | B;
    *(ptr++) = (v >> 21) | B;
    *(ptr++) = v >> 28;
  }
  return reinterpret_cast<char*>(ptr);
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

const char* GetVarint32PtrFallback(const char* p, const char* limit,
                                   uint32_t* value) {
  uint32_t result = 0;
  for (uint32_t shift = 0; shift <= 28 && p < limit; shift += 7) {
    uint32_t byte = *(reinterpret_cast<const unsigned char*>(p));
    p++;
    if (byte & 128) {
      // More bytes are present
      result |= ((byte & 127) << shift);
    } else {
      result |= (byte << shift);
      *value = result;
      return reinterpret_cast<const char*>(p);
    }
  }
  return nullptr;
}

const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* value) {
  uint64_t result = 0;
  for (uint32_t shift = 0; shift <= 63 && p < limit; shift += 7) {
    uint64_t byte = *(reinterpret_cast<const unsigned char*>(p));
    p++;
    if (byte & 128) {
      // More bytes are present
      result |= ((byte & 127) << shift);
    } else {
      result |= (byte << shift);
      *value = result;
      return reinterpret_cast<const char*>(p);
    }
  }
  return nullptr;
}

#define BUF_SIZE 65 * 1024 * 1024
#define PORT 6897
char primary_path[64] = "/mnt/sdb/archive_dbs/sst_dir/sst_last_run/";
char secondary_path[64] = "/mnt/sda/archive_dbs/sst_dir/sst_last_run/";

int copy_sst(uint64_t from, uint64_t to) {
	int fd;
  char *buf = NULL;
	// 1. read primary's sst to buf
  int ret = posix_memalign((void **)&buf, 512, BUF_SIZE);
  if (ret) {
    perror("posix_memalign failed");
    exit(1);
  }
 
	char sst_from[128];
	sprintf(sst_from, "%s%06lu.sst", primary_path, from);
  fd = open(sst_from, O_RDONLY | O_DIRECT, 0755);
  if (fd < 0) {
      perror("open sst failed");
      exit(1);
  }
 
	ret = read(fd, buf, BUF_SIZE);
	if (ret < 0) {
		perror("read sst failed");
	}
  close(fd);
     
  // 2. write buf to secondary's sst   
	char sst_to[1024];
	sprintf(sst_to, "%s%lu", secondary_path, to);
  printf("write to %s\n", sst_to);
  fd = open(sst_to, O_WRONLY | O_DIRECT | O_CREAT, 0755);
  if (fd < 0){
      perror("open sst failed");
      exit(1);
  }
 
	ret = write(fd, buf, BUF_SIZE);
	if (ret < 0) {
		perror("write sst failed");
	}

  close(fd);
  free(buf);
	return 0;
}

int ship_sst(std::vector<uint64_t> sst) { 
	int sock = 0; 
	struct sockaddr_in serv_addr; 
	char buffer[1024] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
		printf("\n Socket creation error \n"); 
		return -1;
	}

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "10.10.1.2", &serv_addr.sin_addr)<=0) { 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
		printf("\nConnection Failed \n"); 
		return -1; 
	}
    
  std::string message;
  std::string delim = " ";
  for (size_t i = 0; i < sst.size(); i++) {
    message += std::to_string(sst[i]) + delim;
  }
  printf("send %s\n", message.c_str());
	send(sock , message.c_str(), strlen(message.c_str()), 0); 

	if (read(sock, buffer, 1024) != 0) {
		printf("recv %s\n", buffer);	
	}

  char *p = strtok(buffer, delim.c_str());
	int i = 0;
  while(p) {
    copy_sst(sst[i++], strtoul(p, NULL, 0));		
    p = strtok(NULL, delim.c_str());
  }

	return 0; 
}

}  // namespace ROCKSDB_NAMESPACE
