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
#define MAP_LENS PAGE_SIZE * 100
#define BUF_SIZE 512
#define HEAD_SIZE 50

#define slave_IOCTL_PRINTPHYSADDR 0x12345676
#define slave_IOCTL_CREATESOCK 0x12345677
#define slave_IOCTL_MMAP 0x12345678
#define slave_IOCTL_EXIT 0x12345679

int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size = 0, data_size = -1, offset = 0;
	int N;
	char file_name[50];
	char method[20];
	char ip[20];
	struct timeval start, end;
	double trans_time; // calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;
	long mmap_size = sysconf(_SC_PAGE_SIZE); // get page size for memory mapping (?)
	fprintf(stderr, "mmap size = %ld\n", mmap_size);
	
	N = atoi(argv[1]);
	fprintf(stderr, "N = %d\n", N);
	strcpy(method, argv[N+2]);
	fprintf(stderr, "method = %s\n", method);
	strcpy(ip, argv[N+3]);
	fprintf(stderr, "ip = %s\n", ip);
	
	if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}
		
	
	size_t total_size = 0;
	
	gettimeofday(&start ,NULL);
	
	for(int n = 0; n < N; n++)
	{
		if(ioctl(dev_fd, slave_IOCTL_CREATESOCK, ip) == -1)	// connect to master in the device
		{
			perror("ioctl create slave socket error\n");
			return 1;
		}
		
		strcpy(file_name, argv[n+2]);
		fprintf(stderr, "file%d = %s\n", n, file_name);
		file_size = 0;
		if( (file_fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
		{
			perror("failed to open input file\n");
			return 1;
		}
		
		switch(method[0])
		{
			case 'f': // fcntl: read()/write()
				while((ret = read(dev_fd, buf, sizeof(buf))) > 0) // read from the the device
				{
					fprintf(stderr, "received %lu: %s\n", ret, buf);
					write(file_fd, buf, ret); //write to the input file
					file_size += ret;
				}
				total_size += file_size;
				break;
			case 'm': // mmap:
				while(1) // read from the the device
				{
					ret = ioctl(dev_fd, slave_IOCTL_MMAP);
					fprintf(stderr, "ret = %lu\n", ret);
					if (ret == 0)
						break;
					posix_fallocate(file_fd, file_size, ret);
					file_address = mmap(NULL, MAP_LENS, PROT_WRITE, MAP_SHARED, file_fd, file_size);
					kernel_address = mmap(NULL, MAP_LENS, PROT_READ, MAP_SHARED, dev_fd, file_size);
					if(kernel_address < 0) {
						perror("kernel address");
						return 1;
					}
					memcpy(file_address, kernel_address, ret);
					file_size += ret;
					total_size += ret;
					fprintf(stderr, "file_size = %lu\n", file_size);
				}
				// finished receiving the file
				ftruncate(file_fd, file_size); // resize the file to correct size
				
				break;
		}
		ioctl(dev_fd, slave_IOCTL_PRINTPHYSADDR, (unsigned long)file_address);
		close(file_fd);
		
		if(ioctl(dev_fd, slave_IOCTL_EXIT) == -1)// end receiving data, close the connection
		{
			perror("ioctl client exits error\n");
			return 1;
		}
	}
	
	
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Transmission time: %lf ms, File size: %lu bytes\n", trans_time, total_size / 8);
	close(dev_fd);
	return 0;
}


