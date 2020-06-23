#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define PAGE_SIZE 4096
#define BUF_SIZE 512
size_t get_filesize(const char* filename);//get the size of the input file


int main (int argc, char* argv[])
{
    int num_files = atoi(argv[1]);

    char buf[BUF_SIZE];
    int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
    size_t ret, file_size, offset = 0, dev_offset = 0, tmp;
    char file_name[num_files][50], method[20];
    char *kernel_address = NULL, *file_address = NULL;
    struct timeval start;
    struct timeval end;
    double trans_time; //calulate the time between the device is opened and it is closed

    int last_argv_index = 2 + num_files;
    for (int i = 2; i < last_argv_index; i++) {
        strcpy(file_name[i-2], argv[i]);
    }
    strcpy(method, argv[last_argv_index]);

    if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
    {
        perror("failed to open /dev/master_device\n");
        return 1;
    }
    int total_filesize = 0;
    gettimeofday(&start ,NULL);

    // This is the line where the process stuck.
    if(ioctl(dev_fd, 0x12345677) == -1) //0x12345677 : create socket and accept the connection from the slave
    {
        perror("ioclt server create socket error\n");
        return 1;
    }


    for (int file_id = 0; file_id < num_files; file_id++) {

        if( (file_fd = open (file_name[file_id], O_RDWR)) < 0 )
        {
        	perror("failed to open input file\n");
        	return 1;
        }

        if( (file_size = get_filesize(file_name[file_id])) < 0)
        {
        	perror("failed to get filesize\n");
        	return 1;
        }
        total_filesize += file_size;
        char *src, *dst;
        
        switch(method[0])
        {
            case 'f': //fcntl : read()/write()
                do {
                    ret = read(file_fd, buf, sizeof(buf)); // read from the input file
                    write(dev_fd, buf, ret);//write to the the device
                } while(ret > 0);
                break;
            case 'm': //mmap
                while (offset < file_size) {
                    if((src = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, file_fd, offset)) == (void *) -1) {
                        perror("mapping input file");
                        return 1;
                    }
                    if((dst = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, dev_fd, dev_offset)) == (void *) -1) {
                        perror("mapping output device");
                        return 1;
                    }
                    do {
                        int len = (offset + BUF_SIZE > file_size ? file_size % BUF_SIZE : BUF_SIZE);
                        memcpy(dst, src, len);
                        offset += len;
                        dev_offset += len;
                        ioctl(dev_fd, 0x12345678, len);
                    } while (offset < file_size && offset % PAGE_SIZE != 0);
                    ioctl(dev_fd, 0x12345676, (unsigned long)src);
                    munmap(src, PAGE_SIZE);
                }
                offset = 0;
                break;
        }
    }

    if(ioctl(dev_fd, 0x12345679) == -1) // end sending data, close the connection
    {
        perror("ioclt server exits error\n");
        return 1;
    }
    gettimeofday(&end, NULL);
    trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
    printf("Transmission time: %lf ms, File size: %ld bytes\n", trans_time, total_filesize / 8);

    close(file_fd);
    close(dev_fd);

    return 0;
}

size_t get_filesize(const char* filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}