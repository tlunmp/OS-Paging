#include "pages.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>	
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <time.h>

int shmid; 
int child_id;
int chance[100];
int chancePos =0;
SharedMemory* shmPtr;

typedef struct message {
    long mType;
    char mtext[512];
} Message;

static int messageQueueId;
int terminate = 0;
void signalCall(int signum);

int main(int argc, char* argv[]) {

	Message message;	

     	if ((shmid = shmget(SHMKEY, sizeof(SharedMemory), 0600)) < 0) {
            perror("Error: shmget");
            exit(errno);
     	}
   
    	 if ((messageQueueId = msgget(MESSAGEKEY, 0644)) == -1) {
            perror("Error: msgget");
            exit(errno);
      	}


 	 
	shmPtr = shmat(shmid, NULL, 0);
		//srand(NULL);
 		//time_t t;
	srand(getpid());

	while(1) {
	
		if (msgrcv(messageQueueId, &message,sizeof(message)+1,1,0) == -1) {
			perror("msgrcv");
		}

		
//		printf("message recieve is %s\n", message.mtext); 
//		printf("\n chance are %d\n",chance);	
	

		int termChance = rand() % (100 + 1 - 1) + 1;
		
		if(termChance >= 80){
			strcpy(message.mtext,"Terminated");
		}
		message.mType = 34;	
		if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
			perror("msgsnd");
			exit(1);
		}
		
		int i = 2;

		for(i =2 ; i < 34; i++){
			int chance = rand() % (100 + 1 - 1) + 1;
			if(chance > 0 && chance < 50) {
				strcpy(message.mtext,"WRITE");
			} else if(chance >= 50 && chance < 101) {
				strcpy(message.mtext,"READ");

			}
			message.mType = i;	
			
		//	strcpy(message.mtext,"Release");
			if(msgsnd(messageQueueId, &message,sizeof(message)+1,0) == -1) {
				perror("msgsnd");
				exit(1);
			} 

		}

		exit(0);
	
  	 	//kill(0, SIGTERM);

	}
	return 0;
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
