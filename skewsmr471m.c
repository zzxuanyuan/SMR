#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <time.h>
#include <signal.h>
#include <asm/fcntl.h>
#include <sys/times.h>
#include <sys/resource.h>

#define TRUE 1
#define FALSE 0
#define BLOCKSIZE 4096

double track_skew(int fd, char *buf, unsigned long sector1, unsigned long sector2){
	struct timespec  start,end;
	int ret,timeret;
	unsigned long diff;
	double steptime;
	unsigned long exptrack;
	lseek(fd,BLOCKSIZE*sector1,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("read error");
	timeret = clock_gettime(CLOCK_REALTIME,&start);
	lseek(fd,BLOCKSIZE*sector2,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) printf("read error");
	timeret = clock_gettime(CLOCK_REALTIME,&end);
	diff = ((start.tv_nsec<end.tv_nsec) ? (end.tv_nsec-start.tv_nsec) : (end.tv_nsec+1000000000-start.tv_nsec));
	steptime = diff/1000000.0;
	printf("steptime is %f\n",steptime);
	return steptime;
}

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
	int fd;
	fd = open(argv[1],O_RDWR|O_SYNC|O_DIRECT);
	if(fd < 0){
		perror("open() failed with error");
		exit(1);
	}
	else{
		printf("open() succeeded\n");
	}

	int ret;
	double skew=0;
	double verify=0;
	unsigned long startsector=471184048;
	int count=0;
	unsigned long currsector=startsector;
	while(currsector<471184048+23000){
		if(currsector%1000==0)	printf("currsector=%lu\n",currsector);
		skew = track_skew(fd,align,currsector,currsector+1);
		verify = track_skew(fd,align,currsector,currsector+1);
		if(abs(verify-skew)<1.0){
			if(skew<9.0)
				printf("Skew!, Current Sector: %lu, Skew: %f\n",currsector,skew);
			else
				printf("No Skew, Current Sector: %lu, Skew: %f\n",currsector,skew);
		} else {
			printf("skew != verify, current Sector: %lu, skew: %f\n", currsector,skew);
		}
		currsector++;
	}
	return 0;
}
