#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>

#include <experimental/filesystem>
#include <vector>
#include <fstream>
#include <iostream>

#include "thread_pool.h"
#include "common.h"

namespace fs = std::experimental::filesystem;

static const char* dirs_to_monitor[] = {compaction_meta_path};

static int num_of_tasks = 0;
 
struct sst_ship_result {
    int                 i;
    bool                finished;
    // std::string   type;
};

struct task{
    
    typedef sst_ship_result result_type;
    task(int i, const char* filename): 
        i_(i),
        filename_(filename){}

    result_type operator()(){
        result_type res;
        res.i = i_;

        const std::string fileName = std::string(compaction_meta_path) + "/" + std::string(filename_);

        // std::cout << "handling " << i_ << " th task, filename : " << fileName << std::endl;
        std::ifstream file;

        file.open(fileName);

        std::string line;
        int line_num = 0;

        while(std::getline(file, line)){
        
            // std::cout << "line " << line_num << " " << line << std::endl;
            std::istringstream iss(line);
            std::string word;

            int word_num = 0;

            std::string cur_first;
            std::string cur_level;
            while(iss >> word) {
                /* do stuff with word */
                // first line 
                // std::cout << " ( word " << word_num << " : " << word << " ) ";
                if (line_num == 0) {
                    if (word_num == 0){

                        if (word.compare("w") == 0) {
                            cur_first = "w";
                            // res.type = "FLUSH";
                        } else {
                            cur_first = "d";
                            // res.type = "COMPACTION";
                        }
                    } else if (word_num == 1){
                        cur_level = word;
                        std::cout << "\n";
                    } else {

                        std::string sst_file = std::string("000000").replace(6 - word.length(), word.length(), word) + ".sst";
                        if (cur_first.compare("w") == 0) {
                            // std::cout << "coping " << std::endl;
                            if (fs::exists(local_sst_dir + sst_file)) {
                                // std::cout << "copying file " << sst_file << std::endl;
                                fs::copy(std::string(local_sst_dir) + sst_file, std::string(remote_sst_dir) + sst_file);
                                // if (fs::exists(remote_sst_dir + sst_file)){
                                //     std::cout << "copy succeeds " << "\n";
                                // }
                            } else {
                                //file does not exist in local sst dir, do not need to copy it
                                // std::cout << "copy failed : ";
                                // std::cout << "file " << sst_file << " does not exist in local sst dir" << std::endl;
                            }
                        } else if (cur_first.compare("d") == 0){
                            // std::cout << "deleting " << std::endl;
                            if (fs::exists(remote_sst_dir + sst_file)){

                                // std::cout << "removing file " << sst_file << std::endl;
                                fs::remove(std::string(remote_sst_dir) + sst_file);
                                // if (!fs::exists(remote_sst_dir + sst_file)){
                                //          std::cout << "remove succeeds" << "\n";                                                     
                                //    }                       
                            } else {
                                // file does not exist in remote sst dir, do not need to remove it 
                                // std::cout << "remove failed : ";
                                // std::cout << "file " << sst_file << " does not exist in remote sst dir " << std::endl;
                            }
                        }
                    }
                    
                } else { // line num >= 1
                    if (word_num == 0){
                        if (word.compare("w") == 0) {
                            cur_first = "w";
                        } else {
                            cur_first = "d";
                        }
                    } else if (word_num == 1){
                        cur_level = word;
                        std::cout << "\n";
                    } else {
                        std::string sst_file = std::string("000000").replace(6 - word.length(), word.length(), word) + ".sst";

                        if (cur_first.compare("w") == 0) {
                            // std::cout << "coping " << std::endl;

                            if (fs::exists(local_sst_dir + sst_file)) {
                                // std::cout << "copying file from " << local_sst_dir +  sst_file  << " to " <<  remote_sst_dir + sst_file << std::endl;

                                fs::copy(std::string(local_sst_dir) + sst_file, std::string(remote_sst_dir) + sst_file);
                                // if (fs::exists(remote_sst_dir + sst_file)){
                                //     std::cout << "copy succeeds " << "\n";
                                // }
                            } else {
                                //file does not exist in local sst dir, do not need to copy it
                            }
                        } else if (cur_first.compare("d") == 0){

                            // std::cout << "deleting " << std::endl;

                            if (fs::exists(remote_sst_dir + sst_file)){
                                // std::cout << "removing file " << sst_file << std::endl;
                                 fs::remove(std::string(remote_sst_dir) + sst_file);   
                                //     if (!fs::exists(remote_sst_dir + sst_file)){
                                //          std::cout << "remove succeeds" << "\n";                                                     
                                //    }
                            } else {
                                // std::cout << "remove failed : " << sst_file << "does not exist " << "\n";
                                // file does not exist in remote sst dir, do not need to remove it 
                            }
                        }
                    }
                }

                word_num++;
            }
            line_num++;
            std::cout << "\n";
        }

        res.finished = true;
        return res;
    }

    int i_;
    const char* filename_;
};



/* Read all available inotify events from the file descriptor 'fd'.
  wd is the table of watch descriptors for the directories in argv.
  argc is the length of wd and argv.
  argv is the list of watched directories.
  Entry 0 of wd and argv is unused. */
static void handle_events(int fd, int *wd, thread_pool<task> *tp)
{
    /* Some systems cannot read integer variables if they are not
      properly aligned. On other systems, incorrect alignment may
      decrease performance. Hence, the buffer used for reading from
      the inotify file descriptor should have the same alignment as
      struct inotify_event. */

    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;

    /* Loop while events can be read from inotify file descriptor. */

    for (;;) {

        /* Read some events. */
        len = read(fd, buf, sizeof(buf));
        
        if (len == -1 && errno != EAGAIN) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* If the nonblocking read() found no events to read, then
          it returns -1 with errno set to EAGAIN. In that case,
          we exit the loop. */

        if (len <= 0)
            break;

        /* Loop over all events in the buffer */

        for (char *ptr = buf; ptr < buf + len;
                ptr += sizeof(struct inotify_event) + event->len) {
            
            event = (const struct inotify_event *) ptr;
            // printf("buffer content : %d \n", event->wd);

            /* Print event type */
            // if(event->mask & IN_CREATE)
            //      printf( "IN_CREATE: " );


            // /* Print the name of the watched directory */
            // for (int i = 0; i < num_of_dirs_to_monitor; ++i) {
            //     if (wd[i] == event->wd) {
            //         printf("%s/",dirs_to_monitor[i]);
            //         break;
            //     }
            // }
            
            // /* Print the name of the file */
            // if (event->len)
            //     printf("%s \n", event->name);

            task t(num_of_tasks, event->name);
            num_of_tasks++;
            tp->run_task(t);
        }
    }
}


int main(int argc, char* argv[])
{
    char buf;
    int fd, i, poll_num;
    int *wd;
    nfds_t nfds;
    struct pollfd fds[2];
    
    boost::asio::io_service ioServ;
    thread_pool<task> tp;
    
    printf("Press ENTER key to terminate.\n");

    /* Create the file descriptor for accessing the inotify API */
    fd = inotify_init1(IN_NONBLOCK);
    if (fd == -1) {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }

    /* Allocate memory for watch descriptors */
    wd = (int *)calloc(num_of_dirs_to_monitor, sizeof(int));
    if (wd == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    /* Mark directories for events
      - file was created */

    static_assert(sizeof(dirs_to_monitor)/sizeof(dirs_to_monitor[0]) == num_of_dirs_to_monitor);
    //right now, only need to monitor one directory
     for (i = 0; i < num_of_dirs_to_monitor; i++) {
        wd[i] = inotify_add_watch(fd, compaction_meta_path,
                                  IN_CREATE);
        if (wd[i] == -1) {
            fprintf(stderr, "Cannot watch '%s': %s\n",
                    argv[i], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    /* Prepare for polling */
    nfds = 2;

    /* Console input */
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    /* Inotify input */
    fds[1].fd = fd;
    fds[1].events = POLLIN;

    /* Wait for events and/or terminal input */
    printf("Listening for events.\n");
    while (1) {

        poll_num = poll(fds, nfds, -1);
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (poll_num > 0) {

            if (fds[0].revents & POLLIN) {

                /* Console input is available. Empty stdin and quit */
                while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n')
                    continue;
                break;
            }

            if (fds[1].revents & POLLIN) {

                /* Inotify events are available */
                handle_events(fd, wd, &tp);
            }
        }
    }

    printf("Listening for events stopped.\n");

    std::vector<sst_ship_result> results = tp.GetCompletedTaskResult();
    printf("number of task to do : %d \n", num_of_tasks );

    printf("number of completed tasks  : %lu \n", results.size());

    /* Close inotify file descriptor */
    close(fd);

    free(wd);
    exit(EXIT_SUCCESS);
}