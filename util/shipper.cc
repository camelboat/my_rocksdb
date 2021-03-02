#include <string.h> 
#include "util/shipper.h"

char sst_path[64] = "/mnt/sda/archive_dbs/sst_dir/sst_last_run/";

int read_sst(uint64_t id, char *buf) {
	int fd;
    int ret = posix_memalign((void **)&buf, 512, BUF_SIZE);
    if (ret) {
        perror("posix_memalign failed");
        exit(1);
    }
 
	char sst_name[128];
	sprintf(sst_name, "%s%06lu.sst", sst_path, id);
    fd = open(sst_name, O_RDONLY, 0755);
    if (fd < 0) {
        perror("open sst failed");
        exit(1);
    }
 
	ret = read(fd, buf, BUF_SIZE);
	if (ret < 0) {
		perror("read sst failed");
	}

	int len = strlen(buf);
	char *pad = new char[BUF_SIZE - len];
	// memset(pad, 0, sizeof(pad));
	strcat(buf, pad);
     
	free(pad);
    close(fd);
	return ret;
}

int copy_sst(uint64_t from, uint64_t to) {
	int fd;
    int ret;
    char *buf = NULL;
	read_sst(from, buf);

	char sst_to[1024];
	sprintf(sst_to, "%s%lu", sst_path, to); 
    fd = open(sst_to, O_WRONLY | O_DIRECT | O_CREAT, 0755);
    if (fd < 0){
        perror("open sst failed");
        exit(1);
    }
 
	ret = write(fd, buf, BUF_SIZE);
	if (ret < 0) {
		perror("write sst failed");
	}
 
    free(buf);
    close(fd);
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