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
//#define BLOCKSIZE 2097152
//#define BLOCKSIZE 8192
//#define BLOCKSIZE 16384
//#define BLOCKSIZE 32768
//#define BLOCKSIZE 65536
#define BLOCKSIZE 4096
//#define BLOCKSIZE 131072
//#define BLOCKSIZE 262144
//#define BLOCKSIZE 524288
//#define BLOCKSIZE 1048576
//#define BLOCKSIZE 134217728

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
		return 1;
	}
	else{
		printf("open() succeeded\n");
	}

	struct timespec start,end;
	int ret,timeret;
	unsigned long diff;
	double steptime;
	unsigned long startsector = 60120;
	unsigned long testsector = 60120;
	unsigned long stepsector = 0;
	memset(align,5,BLOCKSIZE);
	unsigned long i;
//	int step = *argv[2]-'0';
//	printf("step is %d\n",step);
	for(i=0;i<4800;i++){
		lseek(fd,BLOCKSIZE*testsector,SEEK_SET);
		ret = read(fd,align,BLOCKSIZE);
		if(ret<0) perror("read error");
		timeret = clock_gettime(CLOCK_REALTIME,&start);
		lseek(fd,BLOCKSIZE*(testsector-1),SEEK_SET);
		ret = write(fd,align,BLOCKSIZE);
		if(ret<0) printf("write error");
		timeret = clock_gettime(CLOCK_REALTIME,&end);
		diff = ((start.tv_nsec<end.tv_nsec) ? (end.tv_nsec-start.tv_nsec) : (end.tv_nsec+1000000000-start.tv_nsec));
		steptime = diff/1000000.0;
		printf("test:%lu,steptime is %f\n",testsector,steptime);
//		stepsector = rand()%10020;
		stepsector++;
		stepsector = stepsector%9;
		testsector = startsector+stepsector;
//		testsector+=step;
	}

	return 0;
}
