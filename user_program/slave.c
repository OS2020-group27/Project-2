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

#define slave_IOCTL_PRINTPHYSADDR 0x12345676
#define slave_IOCTL_CREATESOCK 0x12345677
#define slave_IOCTL_MMAP 0x12345678
#define slave_IOCTL_EXIT 0x12345679

struct file {
	char name[50];
	size_t size;
};

int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, data_size = -1;
	int N;
	char method[20];
	char ip[20];
	struct timeval start, end;
	double trans_time; // calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;
	struct file *files;
	long mmap_size = sysconf(_SC_PAGE_SIZE); // get page size for memory mapping (?)
	fprintf(stderr, "mmap size = %ld\n", mmap_size);
	
	N = atoi(argv[1]);
	fprintf(stderr, "N = %d\n", N);
	strcpy(method, argv[N+2]);
	fprintf(stderr, "method = %s\n", method);
	strcpy(ip, argv[N+3]);
	fprintf(stderr, "ip = %s\n", ip);
	files = (struct file *)malloc(sizeof(struct file) * N);
	
	if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}
	
	gettimeofday(&start ,NULL);
	
	for(int n = 0; n < N; n++)
	{
		struct file *cur_file = &(files[n]);
		cur_file->size = 0;
		strcpy(cur_file->name, argv[n+2]);
		fprintf(stderr, "file%d = %s\n", n, cur_file->name);
		if( (file_fd = open(cur_file->name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
		{
			perror("failed to open input file\n");
			return 1;
		}

		if(ioctl(dev_fd, slave_IOCTL_CREATESOCK, ip) == -1)	// connect to master in the device
		{
			perror("ioctl create slave socket error\n");
			return 1;
		}

		write(1, "ioctl success\n", 14);
		
		char *dst; // the address of the page in memory
		
		switch(method[0])
		{
			case 'f': // fcntl: read()/write()
				do
				{
					ret = read(dev_fd, buf, sizeof(buf)); // read from the the device
					write(file_fd, buf, ret); //write to the input file
					cur_file->size += ret;
				}while(ret > 0);
				break;
			case 'm': // mmap:
				while((ret = read(dev_fd, buf, sizeof(buf))) > 0) // read from the the device
				{
					if(cur_file->size % mmap_size == 0) // if the page is full
					{
						if(cur_file->size > 0)
						{
							// print physical address of the page
							ioctl(dev_fd, slave_IOCTL_PRINTPHYSADDR, (unsigned long)dst);
							munmap(dst, mmap_size); // remove the old page from memory
						}
						ftruncate(file_fd, cur_file->size+mmap_size); // pre-allocate space for input
						dst = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
						           MAP_SHARED, file_fd, cur_file->size); // allocate a page in memory
						if(dst == (void *) -1)
						{
							perror("mapping output file");
						   return 1;
						}
					}
					memcpy(&dst[cur_file->size%mmap_size], buf, ret); // write to page
					cur_file->size += ret; // increase file size
				}
				// finished receiving the file
				ftruncate(file_fd, cur_file->size); // resize the file to correct size
				// print physical address of the page
				ioctl(dev_fd, slave_IOCTL_PRINTPHYSADDR, (unsigned long)dst);
				munmap(dst, mmap_size); // remove page from memory
				break;
		}

		close(file_fd);
	}
	
	if(ioctl(dev_fd, slave_IOCTL_EXIT) == -1)// end receiving data, close the connection
	{
		perror("ioclt client exits error\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Transmission time: %lf ms, File size: ", trans_time);
	for(int n = 0; n < N; n++)
		printf("%ld bytes%s", files[n].size / 8, (n==N-1)? "\n" : ", ");

	close(dev_fd);
	return 0;
}


