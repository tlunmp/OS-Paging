#include "pages.h"

typedef struct message {
    long myType;
    char mtext[512];
} Message;

int leastUsed = 0;
int allOccupied = 0;
void signalCall(int signum);
int shifting = 0;
int messageQueueId;
int shmid; 
SharedMemory* shmPtr;
Clock launchTime;
int memoryCount=0;
int memoryAddress = 0;
int nonTerminated[18];
int terminatedNumber = 0;

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

void displayFrameTable();
void displayPageTable();
void displaySinglePageTable(int fakePid);

int main(int argc, char* argv[]) {
	int totalCount = 0,
	    requestNumbers = 0,
	    bufSize = 200,
	    timer = 5,
	    maxChildProcess = 100,
	    l,
	    num = 0;

	int c;
	char errorMessage[bufSize],
	     charMessage[bufSize];

	Message message;

	int ptr_count = 0;
	int maxUserProcess = 0;
	
	//getopt command for command line
	while((c = getopt (argc,argv, "hv:n:")) != -1) {

		switch(c) {
			case 'h':
				helpMenu();
				return 1;
		case 'n': maxUserProcess = atoi(optarg);
				break;
			default:
				fprintf(stderr, "%s: Error: Unknown option -%c\n",argv[0],optopt);
				return -1;	
		}

	}

	if(maxUserProcess > 30) {
		fprintf(stderr,"Error: max user process is 18, setting default to 18");
		maxUserProcess = 18;
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
	
	
	//frameTable[0].occupied = 1;	

	inputPageToFrame(isPresent, fakePid, pageAddress, charMessage, randomizePageAddress);

	
	displaySinglePageTable(fakePid);
	displayFrameTable();
*/


	//alarm
	//alarm(timer);
	int o;
	for(o=0; o<18; o++){
		nonTerminated[o] = o;
	}	

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
						
			
							
						for(l=0; l<18;l++){
							if(nonTerminated[l] == -1){
								terminatedNumber++;					
							} 
						}
					
						if(nonTerminated[num] != -1){
							fakePid = nonTerminated[num];
						} else {	
							
							int s = num;
							for(s=num; s<18;s++){
								if(nonTerminated[s] == -1){
									num++;
								} else {
									break;
								}

							}
							
							fakePid = nonTerminated[num];

							//printf("fakeid is %d",fakePid);
						
						}

						//if all terminate it just terminate the program or reset it	
						if(terminatedNumber == 18){					
							fprintf(stderr,"All Process are Terminated.\n");
  	 						
					//		 shmdt(shmPtr); //detaches a section of shared memory
  					//	  	shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
					//		msgctl(messageQueueId, IPC_RMID, NULL); 
 							return 0;
						//kill(0, SIGTERM);
						} else {
							terminatedNumber = 0;
						}
	
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

						if (msgrcv(messageQueueId, &message,sizeof(message)+1,34,0) == -1) {
								perror("msgrcv");

						}	
						if(strcmp(message.mtext, "Terminated") == 0 ){
								fprintf(stderr,"Master: Terminating P%d at %d:%d\n",fakePid, shmPtr->clockInfo.seconds,shmPtr->clockInfo.nanoSeconds);
								fprintf(stderr,"	P%d setting page table present to 0\n",fakePid);
								fprintf(stderr,"	P%d setting frame table occupied,referencebit,dirtyBit to 0\n",fakePid);

								
								nonTerminated[fakePid] = -1;
								int u;
								for(u=0; u <32; u++){
									process[fakePid].pageTable[u].present = 0;
									frameTable[process[fakePid].pageTable[u].frameNo].occupied = 0;	
									frameTable[process[fakePid].pageTable[u].frameNo].referenceBit = 0;	
									frameTable[process[fakePid].pageTable[u].frameNo].dirtyBit = 0;	
								}
									
								if(num < 17){			
									num++;	
								} else {
						
									num = 0;		
								}	

								fakePid = num;
						}

						int m;
						for(m = 2; m <34;m++){	
							if (msgrcv(messageQueueId, &message,sizeof(message)+1,m,0) == -1) {
								perror("msgrcv");

							}	
						//	printf(" operation %s\n", message.mtext);
						
												
							int randomPA = randomizePageAddress();
							int pageAddress = returnPageAddress(randomPA);
							generateLaunch(150000);								
						//	printf("pageaddress %d\n",pageAddress);		
							int isPresent = setPagePresent(fakePid, pageAddress);	
					

							 

							inputPageToFrame(isPresent, fakePid, pageAddress, message.mtext, randomizePageAddress);
							
							memoryCount++;
							if(memoryCount == 100){
								displayFrameTable();
								memoryCount = 0;
							}	
							

						}


				
						if(num < 17){			
							num++;	
						} else {
						
							num = 0;		
						}	
	
					generateLaunch(randomInterval());	
			
			}
			
		}

	 shmdt(shmPtr); //detaches a section of shared memory
    	shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
	msgctl(messageQueueId, IPC_RMID, NULL); 

	return 0;
}



void displaySinglePageTable(int fakePid) {
		
		int j;
		fprintf(stderr, "-----------------------------------\n");
		fprintf(stderr, "|            Process %2d           |\n",fakePid);		
		fprintf(stderr, "-----------------------------------\n");
		fprintf(stderr, "| Table # | Present | FrameNumber |\n");		
		fprintf(stderr, "-----------------------------------\n");
		for(j=0; j < 32; j++){
		fprintf(stderr, "|   %2d    |    %d    |    %3d      |\n", j, process[fakePid].pageTable[j].present, process[fakePid].pageTable[j].frameNo);
			fprintf(stderr, "-----------------------------------\n");
			//printf("process %d, pagetable %d : PresentBit = %d\n",i,j,process[i].pageTable[j].present);
		}
}



void displayPageTable(){


	int i,j;
	for(i=0; i < 18; i++) {
		fprintf(stderr, "-----------------------------------\n");
		fprintf(stderr, "|            Process %2d           |\n",i);		
		fprintf(stderr, "-----------------------------------\n");
		fprintf(stderr, "| Table # | Present | FrameNumber |\n");		
		fprintf(stderr, "-----------------------------------\n");
			for(j=0; j < 32; j++){
				fprintf(stderr, "|   %2d    |    %d    |    %3d      |\n", j, process[i].pageTable[j].present, process[i].pageTable[j].frameNo);
				fprintf(stderr, "-----------------------------------\n");
			//printf("process %d, pagetable %d : PresentBit = %d\n",i,j,process[i].pageTable[j].present);
		}
	}

	
}


void displayFrameTable(){
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
	//	fprintf(stderr,"    %d     |",frameTable[i].pageNo);
	//	fprintf(stderr,"    %d     |",frameTable[i].fakePid);
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
	int i;

	//printf("is present %d, operation %s\n",isPresent, operation);


	if(frameTable[255].occupied == 0){	

		if(isPresent == 1){
			for(i=0; i < 256; i++){
				if(frameTable[i].occupied == 0){

					if(strcmp(operation, "WRITE") == 0){
						frameTable[i].occupied = 1;
						frameTable[i].dirtyBit = 1;
						fprintf(stderr,"Master: Dirty bit of frame %d set, adding additional time to the clock\n",i);
						generateInterval(15000);
						frameTable[i].referenceBit |= (1<<7);
						frameTable[i].fakePid = fakePid;
						frameTable[i].pageNo = pageAddress;
						process[fakePid].pageTable[pageAddress].frameNo = i;
						memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
						fprintf(stderr,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
						generateInterval(15000);
						fprintf(stderr,"Master: Address %d in frame %d writing  data to frame at time %d:%d\n", memoryAddress, i, shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
						fprintf(stderr,"Master: Address %d is not in a frame, pagefault\n",memoryAddress);
						shifting++;
					} else {
						frameTable[i].occupied = 1;
						frameTable[i].fakePid = fakePid;
						frameTable[i].referenceBit |= (1<<7);
						frameTable[i].pageNo = pageAddress;	
						process[fakePid].pageTable[pageAddress].frameNo = i;
						memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
						fprintf(stderr,"Master: P%d is requesting read of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
						generateInterval(15000);
						fprintf(stderr,"Master: Address %d in frame %d giving data to P%d at time %d:%d\n", memoryAddress, i, fakePid, shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
						fprintf(stderr,"Master: Address %d is not in a frame, pagefault\n",memoryAddress);

						shifting++;
					}
					break;
				}
			}	
		//if pagetable is occupid or present
		} else {
			if(strcmp(operation, "WRITE") == 0){
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				fprintf(stderr,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				generateInterval(15000);
				fprintf(stderr,"Master: P%d is updating dirtybit and referenceBit on frame %d\n",fakePid, process[fakePid].pageTable[pageAddress].frameNo);
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].dirtyBit = 1;
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].referenceBit |= (1<<7);
				shifting++;
				//printf("write shifting %d and %d\n",shifting, i);
			} else {
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				fprintf(stderr,"Master: P%d is requesting read of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				generateInterval(15000);
				fprintf(stderr,"Master: P%d is updating dirtybit and referenceBit on frame %d\n",fakePid, process[fakePid].pageTable[pageAddress].frameNo);
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].referenceBit |= (1<<7);
				shifting++;
				//printf("read shifting %d and %d\n",shifting, i);
			}
		}

		if(shifting == 200){
			for(i=0; i < 256;i++){
				frameTable[i].referenceBit >>= 1;
				//printf("shifted number is %d\n", frameTable[i].referenceBit);
		}
			//printf("shifting hit %d",shifting);
			shifting = 0;
		}

	//swapping
	
	} else {

		allOccupied++;
	}


	if(allOccupied > 1){				
		int i, min = frameTable[0].referenceBit;

		for(i = 1; i < 256; i++){
			if(frameTable[i].referenceBit < min){
				leastUsed = i;	
			}
		}

		if(isPresent == 1){
			if(strcmp(operation, "WRITE") == 0){
				
				frameTable[leastUsed].occupied = 1;
				frameTable[leastUsed].dirtyBit = 1;
				fprintf(stderr,"Master: Dirty bit of frame %d set, adding additional time to the clock\n",leastUsed);
				generateInterval(15000);
				frameTable[leastUsed].referenceBit |= (1<<7);
				frameTable[leastUsed].fakePid = fakePid;
				frameTable[leastUsed].pageNo = pageAddress;
				process[fakePid].pageTable[pageAddress].frameNo = leastUsed;
				process[frameTable[leastUsed].fakePid].pageTable[frameTable[leastUsed].pageNo].present = 0;

				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				generateInterval(15000);
				fprintf(stderr,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				shifting++;
				//printf("write shifting %d and %d\n",shifting, i);
			} else {
				frameTable[leastUsed].occupied = 1;
				frameTable[leastUsed].fakePid = fakePid;
				frameTable[leastUsed].referenceBit |= (1<<7);
				frameTable[leastUsed].pageNo = pageAddress;	
				process[fakePid].pageTable[pageAddress].frameNo = leastUsed;
				process[frameTable[leastUsed].fakePid].pageTable[frameTable[leastUsed].pageNo].present = 0;
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				generateInterval(15000);
				fprintf(stderr,"Master: P%d is requesting read of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				shifting++;
				//printf("read shifting %d and %d\n",shifting, i);
			}	
		//if pagetable is occupid or present
		} else {
			if(strcmp(operation, "WRITE") == 0){
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].dirtyBit = 1;
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].referenceBit |= (1<<7);
				process[frameTable[leastUsed].fakePid].pageTable[frameTable[leastUsed].pageNo].present = 0;
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				generateLaunch(15000);
				generateInterval(15000);
				fprintf(stderr,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
			
				shifting++;
				//printf("write shifting %d and %d\n",shifting, i);
			} else {
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].referenceBit |= (1<<7);
				process[frameTable[leastUsed].fakePid].pageTable[frameTable[leastUsed].pageNo].present = 0;
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				generateLaunch(15000);
				generateInterval(15000);
				fprintf(stderr,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
			
	shifting++;
				//printf("read shifting %d and %d\n",shifting, i);
			}
		}

		if(shifting == 200){
			for(i=0; i < 256;i++){
				frameTable[i].referenceBit >>= 1;
				//printf("shifted number is %d\n", frameTable[i].referenceBit);
			}
		//	printf("shifting hit %d",shifting);
			shifting = 0;
		}
		
			

	}

}


//create memory address
int createMemoryAddress(int fakePid, int pageAddress, int randomizePageAddress) {
	//fprintf(stderr,"\nfakePid %d, pageAddress: %d,randomize %d\n",fakePid, pageAddress, randomizePageAddress);	
	int offSet = randomizePageAddress % 1024;
	//fprintf(stderr,"\n%d\n",process[fakePid].pageTable[pageAddress].frameNo);
	int memoryAddress1 = process[fakePid].pageTable[pageAddress].frameNo * 1024 + offSet;
	//fprintf(stderr,"\nprocess %d, pagetable %d, newAddress %d\n", fakePid, pageAddress, memoryAddress1);
	return memoryAddress1;

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
//	printf("%d\n",randomAddress);
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
		printf("-n int		              | int for max user processor\n"); 
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
}
