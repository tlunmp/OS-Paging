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
FILE *fp;
int lineLimit = 0; 

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

int numMemoryAccessed = 0;
int numPageFault = 0;


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
	int maxUserProcess = 18;
	
	fp  = fopen("logfile.txt", "a+");
	
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


	if(maxUserProcess > 18) {
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

	//initialize default frame table
	initializeFrameTable();
	
	//inititalize default page table
	initializeProcessTable();

	//generate launch time
	generateLaunch(randomInterval());

	//alarm
//	alarm(timer);
	
	int o;
	for(o=0; o<maxUserProcess; o++){
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


			if(ptr_count < maxUserProcess && shmPtr->clockInfo.seconds == launchTime.seconds && shmPtr->clockInfo.nanoSeconds > launchTime.nanoSeconds){	
			
					
							

						//check if the process is terminated or not
						for(l=0; l<maxUserProcess;l++){
							if(nonTerminated[l] == -1){
								terminatedNumber++;					
							} 
						}
					
						//check if the process in the araray on the next one is still -1
						if(nonTerminated[num] != -1){
							fakePid = nonTerminated[num];
						} else {	
							
							int s = num;
							for(s=num; s<maxUserProcess;s++){
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
						if(terminatedNumber == maxUserProcess){					
							fprintf(stderr,"\nAll Process are Terminated.\n");
  	 						int memASec =   numMemoryAccessed/shmPtr->clockInfo.seconds;
							int faultASec = numPageFault/shmPtr->clockInfo.seconds;
							int memASpeed = numMemoryAccessed/memASec;
							fprintf(fp,"Memory Access per second %d\n", memASec);	
  	 						fprintf(fp,"Page Fault Access per second %d\n", faultASec);	
  	 						fprintf(fp,"Average Memory Access speed %d\n", memASpeed);	
							

							shmdt(shmPtr); //detaches a section of shared memory
  						  	shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
							msgctl(messageQueueId, IPC_RMID, NULL); 
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

						//if the user message quue said that the user process terminate
						if(strcmp(message.mtext, "Terminated") == 0 ){
								fprintf(fp,"Master: Terminating P%d at %d:%d\n",fakePid, shmPtr->clockInfo.seconds,shmPtr->clockInfo.nanoSeconds);
								lineLimit++;		
								fprintf(fp,"	P%d setting page table present to 0\n",fakePid);
								lineLimit++;		
								fprintf(fp,"	P%d setting frame table occupied,referencebit,dirtyBit to 0\n",fakePid);
								lineLimit++;		

								//setting the array that is is terminated
								nonTerminated[fakePid] = -1;
								int u;

								//clearing the information since it is terminated
								for(u=0; u <32; u++){
									process[fakePid].pageTable[u].present = 0;
									frameTable[process[fakePid].pageTable[u].frameNo].occupied = 0;	
									frameTable[process[fakePid].pageTable[u].frameNo].referenceBit = 0;	
									frameTable[process[fakePid].pageTable[u].frameNo].dirtyBit = 0;	
								}
									//increament the processor since it will not use the fakepid
								if(num < maxUserProcess){			
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
							generateLaunch(1500000);								
						//	printf("pageaddress %d\n",pageAddress);		
							int isPresent = setPagePresent(fakePid, pageAddress);	
					

							//where all the process work
							inputPageToFrame(isPresent, fakePid, pageAddress, message.mtext, randomizePageAddress);
							
							memoryCount++;

							//display if the memory count is 100 display the table
							if(memoryCount == 100){
								displayFrameTable();
								memoryCount = 0;
							}	
							

						}
				
						if(num < maxUserProcess){			
							num++;	
						} else {
						
							num = 0;		
						}	
	
					generateLaunch(randomInterval());	
			
			}
			
		}

  	 int memASec =   numMemoryAccessed/shmPtr->clockInfo.seconds;
	int faultASec = numPageFault/shmPtr->clockInfo.seconds;
	int memASpeed = numMemoryAccessed/memASec;
	fprintf(fp,"Memory Access per second %d\n", memASec);	
  	fprintf(fp,"Page Fault Access per second %d\n", faultASec);	
  	fprintf(fp,"Average Memory Access speed %d\n", memASpeed);	
	fclose(fp);
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
		fprintf(fp, "-----------------------------------\n");
		fprintf(fp, "|            Process %2d           |\n",i);		
		fprintf(fp, "-----------------------------------\n");
		fprintf(fp, "| Table # | Present | FrameNumber |\n");		
		fprintf(fp, "-----------------------------------\n");
			for(j=0; j < 32; j++){
				fprintf(fp, "|   %2d    |    %d    |    %3d      |\n", j, process[i].pageTable[j].present, process[i].pageTable[j].frameNo);
				fprintf(fp, "-----------------------------------\n");
			//printf("process %d, pagetable %d : PresentBit = %d\n",i,j,process[i].pageTable[j].present);
		}
	}

	
}

//display the frame table
void displayFrameTable(){
	fprintf(fp, "---------------------------------------------\n");
	lineLimit++;		
	fprintf(fp, "|           | Occupied | RefByte | DirtyBit |\n");		
	lineLimit++;		
	fprintf(fp, "---------------------------------------------\n");
	lineLimit++;		
	
	int i;
	for(i = 0; i < 256; i++){
		fprintf(fp, "| Frame %3d |",i);
		
		lineLimit++;		
		if(frameTable[i].occupied == 0){
			fprintf(fp, "    No    |");
		} else {
			fprintf(fp, "   Yes    |");
		}

		fprintf(fp,"  %3d    |",frameTable[i].referenceBit);	
		fprintf(fp,"    %d     |",frameTable[i].dirtyBit);
	//	fprintf(stderr,"    %d     |",frameTable[i].pageNo);
	//	fprintf(stderr,"    %d     |",frameTable[i].fakePid);
		fprintf(fp, "\n");
	}

	fprintf(fp, "---------------------------------------------\n");
	lineLimit++;		

}

//store the frame number to the pagetable when it reads or write
void storingPage (int fakePid, int pageAddress, int frameNumber){
		process[fakePid].pageTable[pageAddress].frameNo = frameNumber;
		//printf("process: %d, framenumber is %d\n", fakePid, process[fakePid].pageTable[pageAddress].frameNo);

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

	//if the last table is not occuppied means no swapping involve
	if(frameTable[255].occupied == 0){	

		//if it the present is 0 means it is available, so theres pagfault
		if(isPresent == 1){
			
			numPageFault++;

			//check the first occupied ont he frame and store it there
			for(i=0; i < 256; i++){
	
				if(frameTable[i].occupied == 0){

					//message queue recieve wrrite
					if(strcmp(operation, "WRITE") == 0){
						//write this on the frame table
						frameTable[i].occupied = 1;
						frameTable[i].dirtyBit = 1;
						frameTable[i].referenceBit |= (1<<7);
						frameTable[i].fakePid = fakePid;
						frameTable[i].pageNo = pageAddress;
						process[fakePid].pageTable[pageAddress].frameNo = i;
					
						//creating memory address
						memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
						numMemoryAccessed++;
						fprintf(stderr, "Generating Log File\n");
						fprintf(fp,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
						lineLimit++;		
						generateInterval(150000);
						fprintf(fp,"Master: Address %d is not in a frame, pagefault\n",memoryAddress);
						lineLimit++;		
						fprintf(fp,"Master: Address %d in frame %d writing  data to frame at time %d:%d\n", memoryAddress, i, shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
						lineLimit++;		
						fprintf(fp,"Master: Dirty bit of frame %d set, adding additional time to the clock\n",i);
						lineLimit++;		
						generateInterval(10);
						shifting++;
					//if the message queue recieve READ, leave the dirtybit and just do occupied and to left most bit on referenceBit
					} else {
						fprintf(stderr, "Generating Log File\n");
						frameTable[i].occupied = 1;
						frameTable[i].fakePid = fakePid;
						frameTable[i].referenceBit |= (1<<7);
						frameTable[i].pageNo = pageAddress;	
						process[fakePid].pageTable[pageAddress].frameNo = i;
						numMemoryAccessed++;
						memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
						fprintf(fp,"Master: P%d is requesting read of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
						lineLimit++;		
						generateInterval(150000);
						fprintf(fp,"Master: Address %d in frame %d giving data to P%d at time %d:%d\n", memoryAddress, i, fakePid, shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
						fprintf(fp,"Master: Address %d is not in a frame, pagefault\n",memoryAddress);
						lineLimit++;		
						lineLimit++;		

						shifting++;
					}
					break;
				}
			}	
		//if pagetable is occupid or present
		} else {

			//just checkign the frame table stored and just changing the refbit since it is used and write then put dirty bit to 1
			if(strcmp(operation, "WRITE") == 0){
				fprintf(stderr, "Generating Log File\n");
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				fprintf(fp,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				lineLimit++;		
				generateInterval(10);
				numMemoryAccessed++;
				fprintf(fp,"Master: P%d is updating dirtybit and referenceBit on frame %d\n",fakePid, process[fakePid].pageTable[pageAddress].frameNo);
				lineLimit++;		
				generateInterval(150000);
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].dirtyBit = 1;
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].referenceBit |= (1<<7);
				shifting++;
				//printf("write shifting %d and %d\n",shifting, i);
				
			//just checking the frame table stored and just chanign the refbit. since it is read we dont need to change the dirtybit
			} else {
				fprintf(stderr, "Generating Log File\n");
				lineLimit++;		
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				fprintf(fp,"Master: P%d is requesting read of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				lineLimit++;		
				generateInterval(10);
				numMemoryAccessed++;
				fprintf(fp,"Master: P%d is updating dirtybit and referenceBit on frame %d\n",fakePid, process[fakePid].pageTable[pageAddress].frameNo);
				lineLimit++;		
				generateInterval(150000);
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].referenceBit |= (1<<7);
				shifting++;
				//printf("read shifting %d and %d\n",shifting, i);
			}
		}

		//when memory accessed hit 200 the reference bit change for all frame
		if(shifting == 200){
			for(i=0; i < 256;i++){
				frameTable[i].referenceBit >>= 1;
				//printf("shifted number is %d\n", frameTable[i].referenceBit);
		}
			//printf("shifting hit %d",shifting);
			shifting = 0;
		}

	//swapping, when fram 255 hit 1
	} else {

		allOccupied++;
	}


	if(allOccupied > 1){	
		//geting the least used bit			
		int i, min = frameTable[0].referenceBit;

		for(i = 1; i < 256; i++){
			if(frameTable[i].referenceBit < min){
				leastUsed = i;	
			}
		}

		//if it the present is 0 means it is available, so theres pagefault
		if(isPresent == 1){
			numPageFault++;

			//message queue receive write
			if(strcmp(operation, "WRITE") == 0){
					//write ont his frame table by setting dirtybit = 1, and change refbit to left most
				fprintf(stderr, "Generating Log File\n");
				frameTable[leastUsed].occupied = 1;
				frameTable[leastUsed].dirtyBit = 1;
				frameTable[leastUsed].referenceBit |= (1<<7);
				frameTable[leastUsed].fakePid = fakePid;
				frameTable[leastUsed].pageNo = pageAddress;
				process[fakePid].pageTable[pageAddress].frameNo = leastUsed;
				process[frameTable[leastUsed].fakePid].pageTable[frameTable[leastUsed].pageNo].present = 0;
				numMemoryAccessed++;
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				fprintf(fp,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				lineLimit++;		
				generateInterval(10);
				fprintf(fp,"Master: Dirty bit of frame %d set, adding additional time to the clock\n",leastUsed);
				lineLimit++;		
				generateInterval(150000);
				fprintf(fp,"Master: Clearing frame %d and swapping in p%d page %d\n", leastUsed, fakePid, pageAddress );	
				lineLimit++;		

				shifting++;
				//printf("write shifting %d and %d\n",shifting, i);
			//message queue recieve READ. leave the dirtybit and just do occupied and to left most refbit
			} else {
				fprintf(stderr, "Generating Log File\n");
				lineLimit++;		
				frameTable[leastUsed].occupied = 1;
				frameTable[leastUsed].fakePid = fakePid;
				frameTable[leastUsed].referenceBit |= (1<<7);
				frameTable[leastUsed].pageNo = pageAddress;	
				process[fakePid].pageTable[pageAddress].frameNo = leastUsed;
				process[frameTable[leastUsed].fakePid].pageTable[frameTable[leastUsed].pageNo].present = 0;
				numMemoryAccessed++;
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				generateInterval(150000);
				fprintf(fp,"Master: P%d is requesting read of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				lineLimit++;		
				generateInterval(10);
				fprintf(fp,"Master: Clearing frame %d and swapping in p%d page %d\n", leastUsed, fakePid, pageAddress );	
				lineLimit++;		
				shifting++;
				//printf("read shifting %d and %d\n",shifting, i);
			}	
		//if pagetable is occupid or present
		} else {
			//just checkign the frame table stored and just changing the refbit since it is used and write then put dirty bit to 1
			if(strcmp(operation, "WRITE") == 0){
				fprintf(stderr, "Generating Log File\n");
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].dirtyBit = 1;
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].referenceBit |= (1<<7);
				process[frameTable[leastUsed].fakePid].pageTable[frameTable[leastUsed].pageNo].present = 0;
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				fprintf(fp,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				lineLimit++;		
				generateInterval(10);
				fprintf(fp,"Master: P%d is updating dirtybit and referenceBit on frame %d\n",fakePid, process[fakePid].pageTable[pageAddress].frameNo);	
				lineLimit++;		
				generateInterval(150000);
				numMemoryAccessed++;
				shifting++;
				//printf("write shifting %d and %d\n",shifting, i);
			//just checking the frame table stored and just chanign the refbit. since it is read we dont need to change the dirtybit
			} else {
				fprintf(stderr, "Generating Log File\n");
				frameTable[process[fakePid].pageTable[pageAddress].frameNo].referenceBit |= (1<<7);
				process[frameTable[leastUsed].fakePid].pageTable[frameTable[leastUsed].pageNo].present = 0;
				memoryAddress = createMemoryAddress(fakePid, pageAddress,randomizePageAddress);
				generateInterval(10);
				numMemoryAccessed++;
				fprintf(fp,"Master: P%d is requesting write of the address %d at time %d:%d\n",fakePid, memoryAddress,shmPtr->clockInfo.seconds, shmPtr->clockInfo.nanoSeconds);
				lineLimit++;		
				generateInterval(150000);
				fprintf(fp,"Master: P%d is updating dirtybit and referenceBit on frame %d\n",fakePid, process[fakePid].pageTable[pageAddress].frameNo);
				lineLimit++;		
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

	 int memASec =   numMemoryAccessed/shmPtr->clockInfo.seconds;
	int faultASec = numPageFault/shmPtr->clockInfo.seconds;
	int memASpeed = numMemoryAccessed/memASec;
	fprintf(stderr,"Memory Access per second %d\n", memASec);	
  	fprintf(stderr,"Page Fault Access per second %d\n", faultASec);	
  	fprintf(stderr,"Average Memory Access speed %d\n", memASpeed);	


    //clean up program before exit (via interrupt signal)
    shmdt(shmPtr); //detaches a section of shared memory
    shmctl(shmid, IPC_RMID, NULL);  // deallocate the memory
   
  	 kill(0, SIGTERM);
}
