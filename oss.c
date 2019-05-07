#include "pages.h"

typedef struct message {
    long myType;
    char mtext[512];
} Message;


void signalCall(int signum);

int messageQueueId;
int shmid; 
SharedMemory* shmPtr;
Clock launchTime;

void helpMenu();
void initializeFrameTable();
void initializeProcessTable();
int randomizePageAddress();
int returnPageAddress(int randomPageAddress);
int setPagePresent(int fakePid, int pageAddress);
void inputPageToFrame(int isPresent, int fakePid, int pageAddress, char *operation, int randomizePageAddress);
int  shiftingBits(int shifting);
void storingPage (int fakePid, int pageAddress, int frameNumber);
int createMemoryAddress(int fakePid, int pageAddress, int randomizePageAddress);

int  randomIntervalLaunch();
int  randomInterval();
void generateInterval(int addInterval);
void generateLaunch(int addInterval);

void displayTable();

int main(int argc, char* argv[]) {
	int totalCount = 0,
	    requestNumbers = 0,
	    bufSize = 200,
	    timer = 5,
	    maxChildProcess = 2;

	int c;
	char verbose[100], 
	     errorMessage[bufSize],
	     charMessage[bufSize];

	Message message;

	int ptr_count = 0;
	
	//getopt command for command line
	while((c = getopt (argc,argv, "hv:n:")) != -1) {

		switch(c) {
			case 'h':
				helpMenu();
				return 1;
			case 'v':
				strcpy(verbose, optarg);
				break;
			case 'n': maxChildProcess = atoi(optarg);
				break;
			default:
				fprintf(stderr, "%s: Error: Unknown option -%c\n",argv[0],optopt);
				return -1;	
		}

	}


	//sigalarm error
	if (signal(SIGALRM, signalCall) == SIG_ERR) {
            snprintf(errorMessage, sizeof(errorMessage), "%s: Error: user: signal(): SIGALRM\n", argv[0]);
	     perror(errorMessage);	
         	exit(errno);
     	}
	

	if ((shmid = shmget(SHMKEY, sizeof(SharedMemory), IPC_CREAT | 0600)) < 0) {
        	perror("Error: shmget");
        	exit(errno);
	}
  
 
	if ((messageQueueId = msgget(MESSAGEKEY, IPC_CREAT | 0644)) == -1) {
        	perror("Error: mssget");
       		 exit(errno);
    	}
  
	 shmPtr = shmat(shmid, NULL, 0);
  	 shmPtr->clockInfo.seconds = 0; 
   	 shmPtr->clockInfo.nanoSeconds = 0;  

	pid_t childpid;

	int fakePid = 0;
	shmPtr->clockInfo.nanoSeconds = 0;
	shmPtr->clockInfo.seconds = 0;
	launchTime.seconds = 0;
	launchTime.nanoSeconds = 500000;

	srand(NULL);
 	time_t t;
	srand((unsigned) time(&t));

	initializeFrameTable();
	
	initializeProcessTable();



	generateLaunch(randomInterval());

	printf("launch time %d:%d",launchTime.seconds, launchTime.nanoSeconds);
	printf("max %d\n", maxChildProcess);

/*		
	int randomPA = randomizePageAddress();
	int pageAddress = returnPageAddress(randomPA);
	
	printf("pageaddress %d\n", pageAddress);

	int isPresent = setPagePresent(fakePid, pageAddress);	

	printf("process %d, pagetable %d, presentBit : %d\n", fakePid, pageAddress, process[fakePid].pageTable[pageAddress].present);	
	
	strcpy(charMessage,"WRITE");
	
	//frameTable[0].occupied = 1;	

	inputPageToFrame(isPresent, fakePid, pageAddress, charMessage, randomizePageAddress);
*/

	//alarm
	alarm(timer);


	displayTable();

	fakePid = 0;
	while(totalCount < maxChildProcess || ptr_count > 0){ 					
			
			shmPtr->clockInfo.nanoSeconds += 20000;
			//clock incrementation
			if(shmPtr->clockInfo.nanoSeconds > 1000000000){
				shmPtr->clockInfo.seconds++;
				shmPtr->clockInfo.nanoSeconds -= 1000000000;
			}				
		
			
			if(waitpid(childpid,NULL, WNOHANG)> 0){
				ptr_count--;
			}


			if(ptr_count < 18 && shmPtr->clockInfo.seconds == launchTime.seconds && shmPtr->clockInfo.nanoSeconds > launchTime.nanoSeconds){	
				
						int randomPA = randomizePageAddress();
						int pageAddress = returnPageAddress(randomPA);
				
						printf("process%d, pageaddress %d\n", fakePid, pageAddress);

	
						message.myType = 1;
						char buffer1[100];
						sprintf(buffer1, "%d", fakePid);
						strcpy(message.mtext,buffer1);	
			
						if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
							perror("msgsnd");
							exit(1);
						}

	
						// char buffer1[100];
						// sprintf(buffer1, "%d", totalCount);
						childpid=fork();

						totalCount++;
						ptr_count++;
		
						if(childpid < 0) {
							perror("Fork failed");
						} else if(childpid == 0) {		
							execl("./user", "user",NULL);
							snprintf(errorMessage, sizeof(errorMessage), "%s: Error: ", argv[0]);
	    	 					perror(errorMessage);		
							exit(0);
						} else {
			
						}
			
						if (msgrcv(messageQueueId, &message,sizeof(message)+1,2,0) == -1) {
							perror("msgrcv");

						}	
						
						fprintf(stderr,"%s\n",message.mtext);
			
						fakePid++;
						generateLaunch(randomInterval());	
			}
			
		}

 	 //kill(0, SIGTERM);	

	 shmdt(shmPtr); //detaches a section of shared memory
    	shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
	msgctl(messageQueueId, IPC_RMID, NULL); 

	return 0;
}

void displayTable(){
	fprintf(stderr, "---------------------------------------------\n");
	fprintf(stderr, "|           | Occupied | RefByte | DirtyBit |\n");		
	fprintf(stderr, "---------------------------------------------\n");
	
	int i;
	for(i = 0; i < 256; i++){
		fprintf(stderr, "| Frame %3d |",i);
		
		if(frameTable[i].occupied == 0){
			fprintf(stderr, "    No    |");
		} else {
			fprintf(stderr, "   Yes    |");
		}

		fprintf(stderr,"  %3d    |",frameTable[i].referenceBit);	
		fprintf(stderr,"    %d     |",frameTable[i].dirtyBit);
		fprintf(stderr, "\n");
	}

	fprintf(stderr, "---------------------------------------------\n");

}

//store the frame number to the pagetable when it reads or write
void storingPage (int fakePid, int pageAddress, int frameNumber){
		process[fakePid].pageTable[pageAddress].frameNo = frameNumber;
		printf("process: %d, framenumber is %d\n", fakePid, process[fakePid].pageTable[pageAddress].frameNo);

}


//shifting bits whenever read/write hit 200
int shiftingBits(int shifting){
	int i;
	if(shifting == 200) {
		for(i=0; i < 256;i++){
			frameTable[i].referenceBit >>= 1;
			//printf("shifted number is %d\n", frameTable[i].referenceBit);
		}
		shifting = 0;
	}
	return shifting;

}

void inputPageToFrame(int isPresent, int fakePid, int pageAddress, char *operation, int randomizePageAddress){
	int i, allOccupied, shifting;

	printf("%s\n",operation);
	for(i=0; i < 256; i++){
		if(frameTable[i].occupied == 0){
			if(strcmp(operation,"WRITE") == 0){
				
				if(isPresent == 1){
					frameTable[i].dirtyBit = 1;
					frameTable[i].referenceBit |= (1<<7);
					printf("writing to the frametable %d\n",i);
					printf("pageAddress is %d, reference bit %d, dirty bit %d\n", frameTable[i].pageNo, frameTable[i].referenceBit, frameTable[i].dirtyBit);	
					createMemoryAddress(fakePid,pageAddress, randomizePageAddress);
					allOccupied++;
					shifting++;
				} else{
					frameTable[i].dirtyBit = 1;
					frameTable[i].referenceBit |= (1<<7);
					frameTable[i].pageNo = pageAddress;
					printf("writing to the frametable %d\n",i);
					printf("pageAddress is %d, reference bit %d, dirty bit %d\n", frameTable[i].pageNo, frameTable[i].referenceBit, frameTable[i].dirtyBit);	
					storingPage (fakePid, pageAddress, i);		
					createMemoryAddress(fakePid,pageAddress, randomizePageAddress);
					allOccupied++;
					shifting++;
				}
				break;
			} else {
				frameTable[i].referenceBit |= (1<<7);
				frameTable[i].pageNo = pageAddress;
				printf("reading to the frametable %d\n",i);
				printf("pageAddress is %d, reference bit %d, dirty bit %d\n", frameTable[i].pageNo, frameTable[i].referenceBit, frameTable[i].dirtyBit);	
				storingPage (fakePid, pageAddress, i);		
				createMemoryAddress(fakePid,pageAddress, randomizePageAddress);
				allOccupied++;
				shifting++;
				allOccupied++;
				break;
			}		
		} 
	}


	shifting = shiftingBits(shifting);


	int j, min = frameTable[0].referenceBit, leastUsed = 0;
	//if everything is occupied then use the reference bit to swap
	if(allOccupied == 256){	
		for(j=1;j < 256; j++){
			if(frameTable[j].referenceBit < min){

				min = frameTable[j].referenceBit;
				leastUsed = i;
			}
		}	
		
		if(strcmp(operation,"WRITE") == 0) {
				if(isPresent == 1){
					frameTable[leastUsed].dirtyBit = 1;
					frameTable[leastUsed].referenceBit |= (1<<7);
					printf("writing to the frametable %d\n",i);
					printf("pageAddress is %d, reference bit %d, dirty bit %d\n", frameTable[i].pageNo, frameTable[i].referenceBit, frameTable[i].dirtyBit);	
					storingPage (fakePid, pageAddress, leastUsed);		
					createMemoryAddress(fakePid,pageAddress, randomizePageAddress);
					shifting++;
				} else	{
					frameTable[leastUsed].dirtyBit = 1;
					frameTable[leastUsed].referenceBit |= (1<<7);
					frameTable[leastUsed].pageNo = pageAddress;
					printf("writing to the frametable %d\n",i);
					printf("pageAddress is %d, reference bit %d, dirty bit %d\n", frameTable[i].pageNo, frameTable[i].referenceBit, frameTable[i].dirtyBit);	
					storingPage (fakePid, pageAddress, leastUsed);		
					createMemoryAddress(fakePid,pageAddress, randomizePageAddress);
					shifting++;
				}
		} else {
				frameTable[leastUsed].referenceBit |= (1<<7);
				frameTable[leastUsed].pageNo = pageAddress;
				printf("reading to the frametable %d\n",i);
				printf("pageAddress is %d, reference bit %d, dirty bit %d\n", frameTable[i].pageNo, frameTable[i].referenceBit, frameTable[i].dirtyBit);	
				storingPage (fakePid, pageAddress, leastUsed);		
				createMemoryAddress(fakePid,pageAddress, randomizePageAddress);
				shifting++;
		}
	}
}


//create memory address
int createMemoryAddress(int fakePid, int pageAddress, int randomizePageAddress) {
	
	int offSet = randomizePageAddress % 1024;
	int memoryAddress = process[fakePid].pageTable[pageAddress].frameNo * 1024 + offSet;
	printf("process %d, pagetable %d, newAddress %d", fakePid, pageAddress, memoryAddress);
	return memoryAddress;

}

//setting the pagetable present on or off/ depends
int setPagePresent(int fakePid, int pageAddress){
	

	int isPresent = 0;
	if(process[fakePid].pageTable[pageAddress].present == 0){
		process[fakePid].pageTable[pageAddress].present = 1;
		isPresent = 1;
	}

/*
 	int i,j;
	for(i=0; i < 18; i++) {
		for(j=0; j < 32; j++){
			printf("process %d, pagetable %d : PresentBit = %d\n",i,j,process[i].pageTable[j].present);
		}
	}
*/

	return isPresent;
}


//return page address
int returnPageAddress(int randomPageAddress) {
	int pageAddress = randomPageAddress / 1024;
	return pageAddress;
} 


//randomize from 0 - 32000
int randomizePageAddress() {
	int randomAddress = rand() % (32000 + 0 - 0) + 0;	
	printf("%d\n",randomAddress);
	return randomAddress;
}


void initializeFrameTable(){
	int i;
	for(i = 0; i < 256; i++) {
		frameTable[i].occupied = 0;	
		frameTable[i].referenceBit = 0;	
		frameTable[i].dirtyBit = 0;	
		//printf("frame table %d: occupied: %d, referenceBit %d, dirtyBit %d\n", i, frameTable[i].occupied, frameTable[i].referenceBit,frameTable[i].dirtyBit);
	}
}

void initializeProcessTable() {
	int i,j;
	for(i=0; i < 18; i++) {
		for(j=0; j < 32; j++){
			process[i].pageTable[j].present = 0;
			//printf("process %d, pagetable %d : PresentBit = %d\n",i,j,process[i].pageTable[j].present);
		}
	}
}
//user process launch
int  randomIntervalLaunch() {
	int times = 0; 
	times = rand() % (25000000 + 1 - 1) + 1;
	return times;
}

//launch interval
int  randomInterval() {
	int times = 0; 
	times = rand() % (50000000 + 1 - 1) + 1;
	return times;
}


//interval for user pocess release of terminate or request, deadlock
void generateInterval(int addInterval){
	shmPtr->clockInfo.nanoSeconds += addInterval;
	
	if(shmPtr->clockInfo.nanoSeconds > 1000000000){
		shmPtr->clockInfo.seconds++;
		shmPtr->clockInfo.nanoSeconds -= 1000000000;
	}
}

//generate between process launch
void generateLaunch(int addInterval) {
	launchTime.nanoSeconds += addInterval;
	
	if(launchTime.nanoSeconds > 1000000000){
		launchTime.seconds++;
		launchTime.nanoSeconds -= 1000000000;
	}				
	
		
}

//help meny
void helpMenu() {
		printf("---------------------------------------------------------------| Help Menu |--------------------------------------------------------------------------\n");
		printf("-h help menu\n"); 
		printf("-v input(On/Off)              | On: It will print out all the information to the log.  \n"); 
		printf("			      |	Off: It will print only the deadlock detection and how it is resolve\n");
		printf("-n int		              | int for max processor\n"); 
		printf("------------------------------------------------------------------------------------------------------------------------------------------------------\n");
}
//signal calls
void signalCall(int signum)
{
    int status;
    if (signum == SIGINT)
        printf("\nSIGINT received by main\n");
    else
        printf("\nSIGALRM received by main\n");
 
    while(wait(&status) > 0) {
        if (WIFEXITED(status))  /* process exited normally */
                printf("User prosess exited with value %d\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))   /* child exited on a signal */
                printf("User process exited due to signal %d\n", WTERMSIG(status));
        else if (WIFSTOPPED(status))    /* child was stopped */
                printf("User process was stopped by signal %d\n", WIFSTOPPED(status));
    }


    //clean up program before exit (via interrupt signal)
    shmdt(shmPtr); //detaches a section of shared memory
    shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
   
  	 kill(0, SIGTERM);
      exit(EXIT_SUCCESS);
 }
