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
int returnPageAddress();



int main(int argc, char* argv[]) {
	int totalCount = 0,
	    requestNumbers = 0,
	    bufSize = 200,
	    timer = 5,
	    maxChildProcess = 1;

	int c;
	char verbose[100], 
	     errorMessage[bufSize];

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


	//alarm
	alarm(timer);
/*	
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
						
						fprintf(stderr, "%d\n",totalCount);
						//fprintf(stderr,"%s\n",message.mtext);
						exit(0);
			}
		}

 	 //kill(0, SIGTERM);
*/	return 0;
}

void displayTable(){

}

int returnPageAddress() {
	int pageAddress = rand() % (32000 + 0 - 0) + 0;
	return pageAddress;
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
			printf("process %d, pagetable %d : PresentBit = %d\n",i,j,process[i].pageTable[j].present);
		}
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
                printf("User process exited with value %d\n", WEXITSTATUS(status));
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
