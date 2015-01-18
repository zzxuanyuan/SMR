#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
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

void log_exit(int sig){
	printf("user interrupt!\n");
	exit(1);
}

double track_skew(int fd, char *buf, unsigned long sector1, unsigned long sector2){
	struct timespec	start,end;
	int ret,timeret;
	unsigned long diff;
	double steptime;
	unsigned long exptrack;
	lseek(fd,BLOCKSIZE*sector1,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("coarse_boundary: read error\n");
	timeret = clock_gettime(CLOCK_REALTIME,&start);
	lseek(fd,BLOCKSIZE*sector2,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("coarse_boundary: read error\n");
	timeret = clock_gettime(CLOCK_REALTIME,&end);
	diff = ((start.tv_nsec<end.tv_nsec) ? (end.tv_nsec-start.tv_nsec) : (end.tv_nsec+1000000000-start.tv_nsec));
	steptime = diff/1000000.0;
	return steptime;
}
double get_rpm(int fd, char *buf){

	unsigned long drt;
	double ddrt;
	struct timespec start;
	struct timespec end;
	int timeret;
	fflush(stdout);
	int ret;
	lseek(fd,0,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) printf("read error\n");
	timeret = clock_gettime(CLOCK_REALTIME,&start);
	//	printf("start.tv_nsec is %lu\n",(unsigned long)start.tv_nsec);
	lseek(fd,0,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("read error\n");
	lseek(fd,0,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("read error\n");
	lseek(fd,0,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("read error\n");
	lseek(fd,0,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("read error\n");
	lseek(fd,0,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("read error\n");

	timeret = clock_gettime(CLOCK_REALTIME,&end);

	drt = (end.tv_sec*1000000000+end.tv_nsec)-(start.tv_sec*1000000000+start.tv_nsec);
	ddrt = drt/1000000.0/5.0;
	return ddrt;
}

unsigned long fine_boundary(int fd, char *buf, unsigned long exptrack, int range, double drt){
	struct timespec start,end;
	int ret,timeret;
	unsigned long startsector=exptrack-range;
	unsigned long endsector=exptrack+range;//run from startsector to endsector-1
	unsigned long diff;
	double difftime;
	lseek(fd,BLOCKSIZE*startsector,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("fine_boundary: read error\n");
	unsigned long cnt = startsector;
	while(cnt<endsector){
		lseek(fd,BLOCKSIZE*cnt,SEEK_SET);
		ret = read(fd,buf,BLOCKSIZE);
		if(ret<0) perror("fine_boundary: read error\n");
		timeret = clock_gettime(CLOCK_REALTIME,&start);
		lseek(fd,BLOCKSIZE*(cnt+1),SEEK_SET);
		ret = read(fd,buf,BLOCKSIZE);
		if(ret<0) perror("fine_boundary: read error\n");
		timeret = clock_gettime(CLOCK_REALTIME,&end);
		diff = ((start.tv_nsec<end.tv_nsec) ? (end.tv_nsec-start.tv_nsec) : (end.tv_nsec+1000000000-start.tv_nsec));
		difftime = diff/1000000.0;
		if(difftime < (drt-1.5)){
			printf("DiffTime=%0.2f(ms), EndofTrack=%lu\n",difftime,cnt);
			return cnt;
		}
		cnt++;
	}
	return 0;
}

unsigned long coarse_boundary(int fd, char *buf, unsigned long startsector, int step, double drt){
	struct timespec	start,end;
	int ret,timeret;
	unsigned long diff;
	double steptime;
	unsigned long exptrack;
	lseek(fd,BLOCKSIZE*startsector,SEEK_SET);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("coarse_boundary: read error\n");
	timeret = clock_gettime(CLOCK_REALTIME,&start);
	lseek(fd,BLOCKSIZE*(step-1),SEEK_CUR);
	ret = read(fd,buf,BLOCKSIZE);
	if(ret<0) perror("coarse_boundary: read error\n");
	timeret = clock_gettime(CLOCK_REALTIME,&end);
	diff = ((start.tv_nsec<end.tv_nsec) ? (end.tv_nsec-start.tv_nsec) : (end.tv_nsec+1000000000-start.tv_nsec));
	steptime = diff/1000000.0;
	exptrack = drt/steptime*step;
	return (startsector+exptrack);
}

int main(int argc, char *argv[]){
	if(argc != 3){
		printf("\n Usage : \n");
		return 1;
	}

/*	int ret;
	ret = remove(argv[2]);
	if(ret<0){
		perror("delete error");
		exit(1);
	}else{
		printf("delete %s succeed\n",argv[2]);
	}
*/
	char command[255];
	sprintf(command,"sudo rm %s\n",argv[2]);
	system(command);

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

	FILE *logfile=NULL;

	logfile = fopen(argv[2],"a+");

	(void) signal(SIGINT, log_exit);

	char *buf;
	char *align;
	buf = (char *)malloc(BLOCKSIZE+BLOCKSIZE-1);
	align = (char *)(((long)buf+BLOCKSIZE-1)/BLOCKSIZE*BLOCKSIZE);
	int fd;
	fd = open(argv[1],O_RDWR|O_SYNC|O_DIRECT);
	if(fd < 0){
		perror("open() failed\n");
		exit(1);
	}
	else{
		printf("open() succeeded\n");
	}

	double drt;
	drt = get_rpm(fd,align);
	if(logfile)	fprintf(logfile,"DRT=%0.2f\n\n",drt);

	if(logfile)	fprintf(logfile,"%15s,%25s,%25s\n","ZoneStart","TrackSize","ZoneEnd");

	unsigned long coarsetrack=8626;
	unsigned long exactboundary=0;
	unsigned long expectboundary=0;
	unsigned long tracksize=0;
	unsigned long currsector=0;
	unsigned long trackstep=1;
	double skew=0;
	double verify=0;
	double hsverify1=0;
	double hsverify2=0;
	int mode=INCREMENT;
	unsigned long zoneboundary=358434604;
	unsigned long startzone=0;
	int shingled=1;
	int range=32;

	while(currsector<1220942647){
		if(currsector==0){
			startzone = zoneboundary;
		} else {
			startzone = zoneboundary +1;
		}
		if(logfile)	fprintf(logfile,"%15lu",startzone);
//		coarsetrack = coarse_boundary(fd,align,startzone,100,drt);
		expectboundary = startzone + coarsetrack;
		range = 32;
//		printf("expect boundary: %lu\n",expectboundary);
		exactboundary = fine_boundary(fd,align,expectboundary,range,drt);
		while(!exactboundary){
			range = range << 1;
			exactboundary = fine_boundary(fd,align,expectboundary,range,drt);	
//			if(logfile)	fprintf(logfile,"\nfine_boundary error, redo it with range %d\n",range);
			printf("fine boundary error, redo it with range %d\n",range);
		}
//		if(logfile)	fprintf(logfile,"%25lu",exactboundary);
		tracksize = exactboundary-startzone+1;

		if(logfile)	fprintf(logfile,"%25lu",tracksize);

		trackstep=1;
		mode = INCREMENT;
		currsector = startzone + tracksize;

		/*flush the cache for some types of disks*/
		while(1){


			if(mode==INCREMENT){
				skew = track_skew(fd,align,currsector-1,currsector);
				verify = track_skew(fd,align,currsector-1,currsector);
				if(abs(skew-verify)<1.0){
					if(skew<(drt-2.0)){
						trackstep = trackstep << 1;
						currsector = currsector + trackstep*tracksize;
					} else {
						mode = DECREMENT;
						trackstep = trackstep >> 1;
						currsector = currsector - trackstep*tracksize;
					}
				}
			} else if(mode=DECREMENT){
				if(trackstep==0){	
					skew = track_skew(fd,align,currsector-1,currsector);
					verify = track_skew(fd,align,currsector-1,currsector);
					if(abs(skew-verify)<1.0){
						if(skew>(drt+1.0)){
//							printf("here is a head switch, verify it\n");
                                                        hsverify1 = track_skew(fd,align,currsector-tracksize-1,currsector-tracksize);
                                                        hsverify2 = track_skew(fd,align,currsector+tracksize-1,currsector+tracksize);
                                                        if((hsverify1<(drt-2.0))&&((hsverify2-drt)<1.0)){
                                                                printf("Head Switch\n");
                                                        } else {
                                                                printf("No Head Switch\n");
                                                        }
							zoneboundary = currsector-1;
						} else if(skew<(drt-2.0)){
							zoneboundary = currsector-1;
						} else {
							zoneboundary = currsector-tracksize-1;
						}
						if(logfile)	fprintf(logfile,"%25lu\n",zoneboundary);
						printf("Zone Boundary Type0: %lu\n",zoneboundary);
						break;
					}
				} else {
					skew = track_skew(fd,align,currsector-1,currsector);
					verify = track_skew(fd,align,currsector-1,currsector);
					if(abs(skew-verify)<1.0){
						if(skew>(drt+1.0)){
							printf("might be head switch, drt=%.2f\n",skew);
							hsverify1 = track_skew(fd,align,currsector-tracksize-1,currsector-tracksize);
							hsverify2 = track_skew(fd,align,currsector+tracksize-1,currsector+tracksize);
							if((hsverify1<(drt-2.0))&&((hsverify2-drt)<1.0)){
								printf("Head Switch\n");
							} else {
								printf("No Head Switch\n");
							}
							zoneboundary = currsector-1;
							if(logfile)	fprintf(logfile,"%25lu\n",zoneboundary);
							printf("Zone Boundary Type1: %lu\n",zoneboundary);
							break;
						} else if(skew<(drt-2.0)){
							trackstep = trackstep >> 1;
							currsector = currsector + trackstep*tracksize;
						} else {
							trackstep = trackstep >> 1;
							currsector = currsector - trackstep*tracksize;
						}
					}
				}
			} else {
				printf("error: it is neither INCREMENT nor DECREMENT\n");
			}
		}
		coarsetrack = tracksize;
		printf("Coarse Track Size: %lu\n",coarsetrack);
	}
	fclose(logfile);
	return 0;
}
