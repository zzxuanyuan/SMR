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

#define MODE_PREDICTION 0
#define MODE_LOW 1

#define INCREMENT 0
#define DECREMENT 1

double get_rpm(int fd, char *buf){

        unsigned long drt;
        double ddrt;
        struct timeval start;
        struct timeval end;
        printf("check the revolution time from disk\n");
        fflush(stdout);
        int ret;
        lseek(fd,0,SEEK_SET);
        ret = read(fd,buf,BLOCKSIZE);
        if(ret<0) printf("read error\n");
        gettimeofday(&start, NULL);
        //      printf("start.tv_usec is %lu\n",(unsigned long)start.tv_usec);
        lseek(fd,0,SEEK_SET);
        ret = read(fd,buf,BLOCKSIZE);
        if(ret<0) printf("read error\n");
        lseek(fd,0,SEEK_SET);
        ret = read(fd,buf,BLOCKSIZE);
        if(ret<0) printf("read error\n");
        lseek(fd,0,SEEK_SET);
        ret = read(fd,buf,BLOCKSIZE);
        if(ret<0) printf("read error\n");
        lseek(fd,0,SEEK_SET);
        ret = read(fd,buf,BLOCKSIZE);
        if(ret<0) printf("read error\n");
        lseek(fd,0,SEEK_SET);
        ret = read(fd,buf,BLOCKSIZE);
        if(ret<0) printf("read error\n");

        gettimeofday(&end, NULL);
        //      printf("end.tv_usec is %lu\n",(unsigned long)end.tv_usec);

        drt = (end.tv_sec*1000000+end.tv_usec)-(start.tv_sec*1000000+start.tv_usec);
        ddrt = drt/1000.0/5.0;
        printf("drt is %0.2f\n",ddrt);
        //      drt = drt/1000;
        //      printf("drt is %lu\n",drt);
        return ddrt;
}

double track_skew(int fd, char *buf, unsigned long sector1, unsigned long sector2){
	struct timeval  start,end;
	int ret;
	unsigned long diff;
	double steptime;
	unsigned long exptrack;
	lseek(fd,BLOCKSIZE*sector1,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("read error");
	gettimeofday(&start,NULL);
	lseek(fd,BLOCKSIZE*sector2,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) printf("read error");
	gettimeofday(&end,NULL);
	diff = ((start.tv_usec<end.tv_usec) ? (end.tv_usec-start.tv_usec) : (end.tv_usec+1000000-start.tv_usec));
//	printf("diff is %lu\n",diff);
	steptime = diff/1000.0;
//	printf("steptime is %f\n",steptime);
	return steptime;
}

unsigned long fine_boundary(int fd, char *buf, unsigned long expsector, int range, double drt){
        struct timeval start,end;
        int ret;
        unsigned long startsector=expsector-range;
        unsigned long endsector=expsector+range;//run from startsector to endsector-1
        unsigned long diff=0;
	unsigned long verify1=0;
	unsigned long verify2=0;
        lseek(fd,BLOCKSIZE*startsector,SEEK_SET);
        ret = read(fd,buf,BLOCKSIZE);
        if(ret<0) printf("fine_boundary: read error\n");
        unsigned long cnt = startsector;
        while(cnt<endsector){
                lseek(fd,BLOCKSIZE*cnt,SEEK_SET);
                ret = read(fd,buf,BLOCKSIZE);
                if(ret<0) printf("fine_boundary: read error\n");
                gettimeofday(&start,NULL);
                lseek(fd,BLOCKSIZE*(cnt+1),SEEK_SET);
                ret = read(fd,buf,BLOCKSIZE);
                if(ret<0) printf("fine_boundary: read error\n");
                gettimeofday(&end,NULL);
                diff = ((start.tv_usec<end.tv_usec) ? (end.tv_usec-start.tv_usec) : (end.tv_usec+1000000-start.tv_usec));
                //              printf("diff is %lu\n",diff);
                if((double)diff/1000.0 < (drt-4.5)){
			printf("diff time: %f,cnt:%lu\n",(double)diff/1000.0, cnt);
			return cnt;
		} 
		if(cnt%1000==0) printf("cnt=%lu\n",cnt);
                cnt++;
        }
        return 0;
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

	double drt = get_rpm(fd,align);

	int ret;
//	int tracksize=10020;
//	int tracksize=10480;
//	int tracksize=20500;
//	int tracksize=9740;
//	int tracksize=9680;
//	int tracksize=10560;
//	int tracksize=10200;
//	int tracksize=9740;
//	int tracksize=9600;
//	int tracksize=9120;
	int tracksize=11453;
	
	double skew=0;
	double verify=0;
//	unsigned long zoneboundary=0;
//	unsigned long zoneboundary=4161399;
//	unsigned long zoneboundary=4674901;
//	unsigned long zoneboundary=4695121;
//	unsigned long zoneboundary=4704861;
//	unsigned long zoneboundary=9045321;
//	unsigned long zoneboundary=9065381;
//	unsigned long zoneboundary=11483921;
//	unsigned long zoneboundary=11504861;
//	unsigned long zoneboundary=13796381;
//	unsigned long zoneboundary=13817141;
//	unsigned long zoneboundary=16173341;
//	unsigned long zoneboundary=16193281;
//	unsigned long zoneboundary=18745021;
//	unsigned long zoneboundary=18869341;
	unsigned long zoneboundary=46799127;

	unsigned long exactboundary=0;
	unsigned long startzone=0;
//	unsigned long currsector=0;
	unsigned long currsector=46799127;
	struct timeval start,end;
	printf("come here\n");
	while(1){
		if(currsector==46799127){
//			startzone=zoneboundary;
//			currsector = startzone+tracksize-1;
			currsector = currsector+tracksize-1;
		} else {
			startzone=zoneboundary+1;
			printf("test currsector:%lu\n",currsector);
			exactboundary = fine_boundary(fd,align,currsector,(tracksize-1),drt);
//			exactboundary = fine_boundary(fd,align,currsector,32,drt);

			if(!exactboundary) printf("did not find exactboundary\n");
			else{
				printf("exactboundary first %lu\n",exactboundary);
				tracksize=exactboundary-startzone+1;
				currsector=startzone+tracksize-1;
			}
		}
		printf("startzone:%lu,zoneboundary:%lu,exactboundary:%lu,currsector:%lu\n",startzone,zoneboundary,exactboundary,currsector);
		while(1){
			skew = track_skew(fd,align,currsector,currsector+1);
			verify = track_skew(fd,align,currsector,currsector+1);
			if(abs(skew-verify)<1.0){
				if(skew<9.0){
					printf("there is a track skew at %lu, skew=%f\n",currsector,skew);
					currsector = currsector+tracksize;
				}
				else{
					printf("there is no track skew at %lu, skew=%f\n",currsector,skew);
					zoneboundary = currsector-tracksize;
					break;
				}
			}
			else{
				printf("verify1 != skew\n");
				break;
			}
		}
	}

	return 0;
}
