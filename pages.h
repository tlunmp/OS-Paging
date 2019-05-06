#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>	
#include <semaphore.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <time.h>

#define SHMKEY 9784
#define MESSAGEKEY 3000
#define MAXPROCESSES 18
#define RESOURCESAMT 20

typedef struct clock {
	int seconds;
	int nanoSeconds;
} Clock;


typedef struct frameTable {
	int dirtyBit;
	int referenceBit;
	
} FrameTable;


typedef struct pageTable {
	int pages[32];
	
} PageTable;

PageTable pageTable[MAXPROCESSES];
FrameTable frameTable[256];

typedef struct shared_memory_object {
    Clock clockInfo;
} SharedMemory; 
