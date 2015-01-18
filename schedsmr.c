#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/fcntl.h>
#include <sched.h>
#include <time.h>
#include <string.h>
#include <signal.h>
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

	char *buf1;
	char *buf2;
	char *buf3;
	char *buf4;
	char *align;
	char *vline1;
	char *vline2;
	char *vline3;
	buf1 = (char *)malloc(BLOCKSIZE+BLOCKSIZE-1);
	buf2 = (char *)malloc(BLOCKSIZE+BLOCKSIZE-1);
	buf3 = (char *)malloc(BLOCKSIZE+BLOCKSIZE-1);
	buf4 = (char *)malloc(BLOCKSIZE+BLOCKSIZE-1);

	align = (char *)(((long)buf1+BLOCKSIZE-1)/BLOCKSIZE*BLOCKSIZE);
	vline1 = (char *)(((long)buf2+BLOCKSIZE-1)/BLOCKSIZE*BLOCKSIZE);
	vline2 = (char *)(((long)buf3+BLOCKSIZE-1)/BLOCKSIZE*BLOCKSIZE);
	vline3 = (char *)(((long)buf4+BLOCKSIZE-1)/BLOCKSIZE*BLOCKSIZE);

	int random = rand();
	memset(align,random,BLOCKSIZE);
	memset(vline1,0,BLOCKSIZE);
	memset(vline2,0,BLOCKSIZE);
	memset(vline3,0,BLOCKSIZE);

	int fd;
	fd = open(argv[1],O_RDWR|O_SYNC|O_DIRECT);
	if(fd < 0){
		perror("open() failed with error");
		exit(1);
	}
	else{
		printf("open() succeeded\n");
	}

	int ret,timeret;
	struct timespec start,end,verify;
	double skew=0;
	unsigned long startsector=46799127;
	int count=0;

	unsigned long sector2=1180;
	unsigned long sector1=35;
	unsigned long sector3=1920;
/*
	unsigned long sector2=300000000;
	unsigned long sector1=100000000;
	unsigned long sector3=200000000;
*/
	unsigned long diff;
	double steptime;
	int val1=0,val2=0,val3=0;

	lseek(fd,BLOCKSIZE*startsector,SEEK_SET);
	ret = read(fd,align,BLOCKSIZE);
	if(ret<0){
		perror("read error\n");
		exit(1);
	}

	int k;
	for(k=0;k<500;++k){
		lseek(fd,BLOCKSIZE*100,SEEK_CUR);
		ret = write(fd,align,BLOCKSIZE);
	}

	timeret = clock_gettime(CLOCK_REALTIME,&start);
	lseek(fd,BLOCKSIZE*(sector1-1),SEEK_SET);
	ret = write(fd,align,BLOCKSIZE);
	lseek(fd,BLOCKSIZE*(sector2-1),SEEK_SET);
	ret = write(fd,align,BLOCKSIZE);
	lseek(fd,BLOCKSIZE*(sector3-1),SEEK_SET);
	ret = write(fd,align,BLOCKSIZE);
	timeret = clock_gettime(CLOCK_REALTIME,&end);

        diff = ((start.tv_nsec<end.tv_nsec) ? (end.tv_nsec-start.tv_nsec) : (end.tv_nsec+1000000000-start.tv_nsec));
        steptime = diff/1000000.0;
        printf("write is %f\n",steptime);

	lseek(fd,BLOCKSIZE*(startsector-1),SEEK_SET);
	ret = read(fd,align,BLOCKSIZE);
	if(ret<0){
		perror("read error\n");
		exit(1);
	}

	int i;
	for(i=0;i<500;++i){
		lseek(fd,BLOCKSIZE*100,SEEK_CUR);
		ret = write(fd,align,BLOCKSIZE);
	}

	timeret = clock_gettime(CLOCK_REALTIME,&start);
	lseek(fd,BLOCKSIZE*(sector1-1),SEEK_SET);
	ret = read(fd,vline1,BLOCKSIZE);
	lseek(fd,BLOCKSIZE*(sector2-1),SEEK_SET);
	ret = read(fd,vline2,BLOCKSIZE);
	lseek(fd,BLOCKSIZE*(sector3-1),SEEK_SET);
	ret = read(fd,vline3,BLOCKSIZE);

	if(memcmp(align,vline1,BLOCKSIZE)==0)	val1=1;
	if(memcmp(align,vline2,BLOCKSIZE)==0)	val2=1;
	if(memcmp(align,vline3,BLOCKSIZE)==0)	val3=1;

	timeret = clock_gettime(CLOCK_REALTIME,&verify);

        diff = ((start.tv_nsec<verify.tv_nsec) ? (verify.tv_nsec-start.tv_nsec) : (verify.tv_nsec+1000000000-start.tv_nsec));
        steptime = diff/1000000.0;
        printf("read is %f\n",steptime);

	if(val1)
		printf("vline1 = align\n");
	if(val2)
		printf("vline2 = align\n");
	if(val3)
		printf("vline3 = align\n");

	return 0;
}
