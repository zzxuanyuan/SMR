#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sched.h>
#include <time.h>
#include <signal.h>
#include <asm/fcntl.h>
#include <sys/times.h>
#include <sys/resource.h>

#define TRUE 1
#define FALSE 0
#define BLOCKSIZE 4096

int main(int argc, char *argv[]){
	if(argc != 2){
		printf("\n Usage : \n");
		return 1;
	}

        //set priority of process to highest. 
        if (setpriority(PRIO_PROCESS,getpid(),-20)!=0)
        {
                printf("Warning: cannot renice.\nTo work better you should run this program as root.\n");
        }

        //set process to real-time
        struct sched_param p;
        p.sched_priority = 10;

        if( sched_setscheduler(0, SCHED_FIFO, &p ) )
        {
                printf("Warning: cannot set process to real-tme.\n");
        }

	char *buf;
	char *align;
	buf = (char *)malloc(BLOCKSIZE+BLOCKSIZE-1);
	align = (char *)(((long)buf+BLOCKSIZE-1)/BLOCKSIZE*BLOCKSIZE);
	memset(align,0,BLOCKSIZE);
	int fd;
	fd = open(argv[1],O_RDWR|O_SYNC|O_DIRECT);
	if(fd < 0){
		perror("open() failed with error");
		exit(1);
	}
	else{
		printf("open() succeeded\n");
	}

	struct timespec start,end;
	int ret,timeret;
	unsigned long diff;
	double steptime;
	unsigned long exptrack;
	int startsector = 0;
	int endsector = 1;
	int cnt=0;
	while(endsector<10030){
		lseek(fd,BLOCKSIZE*startsector,SEEK_SET);
		ret = read(fd,align,BLOCKSIZE);
		if(ret<0) perror("read1 error");
		timeret = clock_gettime(CLOCK_REALTIME,&start);
		lseek(fd,BLOCKSIZE*(endsector-1),SEEK_SET);
		ret = read(fd,align,BLOCKSIZE);
		if(ret<0) perror("read2 error");
		timeret = clock_gettime(CLOCK_REALTIME,&end);
		diff = ((start.tv_nsec<end.tv_nsec) ? (end.tv_nsec-start.tv_nsec) : (end.tv_nsec+1000000000-start.tv_nsec));
		steptime = diff/1000000.0;
		printf("start:%d,end:%d,steptime is %f\n",startsector,endsector,steptime);
		endsector++;
		cnt++;
	}

	return 0;
}
