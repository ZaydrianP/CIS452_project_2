#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>

const int FLOUR = 0;
const int SUGAR = 1;
const int YEAST = 2;
const int BAKING_SODA = 3;
const int SALT = 4;
const int CINNAMON = 5;
const int EGGS = 6;
const int MILK = 7;
const int BUTTER = 8;

const int MIXER = 0;
const int PANTRY = 1;
const int REFRIGERATOR = 2;
const int BOWL = 3;
const int SPOON = 4;
const int OVEN = 5;

const int cookieRecipe[] = {FLOUR, SUGAR, MILK, BUTTER};
const int pancakeRecipe[] = {FLOUR, SUGAR, BAKING_SODA, SALT, EGGS, MILK, BUTTER};
const int pizzaDoughRecipe[] = {YEAST, SUGAR, SALT};
const int softPretzelRecipe[] = {FLOUR, SUGAR, SALT, YEAST, BAKING_SODA, EGGS};
const int cinnamonRollRecipe[] = {FLOUR, SUGAR, SALT, BUTTER, EGGS, CINNAMON};

const int pantryIngredients[] = {FLOUR, SUGAR, YEAST, BAKING_SODA, SALT, CINNAMON};
const int refrigeratorIngredients[] = {EGGS, MILK, BUTTER};

struct semaphoresStruct {
	int length;
	int *semaphoreIds;
};

struct sharedMem {
	int id;
	long int *address;
};

struct sharedMemStruct {
	int length;
	struct sharedMem *sharedMemoryAddresses;
};

struct semaphoresStruct semaphores;

struct sharedMemStruct sharedMemory;

int insertIntoSemaphoreArray(int resource, int semaphoreId) {
	if(resource >= semaphores.length) {
		int *temp = realloc(semaphores.semaphoreIds, (resource + 1) * sizeof(int));

		if (temp == NULL) {
			perror("Failed to allocate memory for semaphoreIds");
			return -1; // Memory allocation failure
		}

		semaphores.semaphoreIds = temp;
		semaphores.length = resource + 1;

	}

	semaphores.semaphoreIds[resource] = semaphoreId; // Add the new element
	return 0; // Success
	
}
//TODO: Adjust this function based on what the sharedMemory represents. 
int insertIntoSharedMemArray(struct sharedMem memoryAddress) {
	struct sharedMem *temp = realloc(sharedMemory.sharedMemoryAddresses, 
			(sharedMemory.length + 1) * sizeof(struct sharedMem));

	if (temp == NULL) {
		perror("Failed to allocate memory for sharedMemoryAddresses");
		return -1; // Memory allocation failure
	}

	sharedMemory.sharedMemoryAddresses = temp;
	sharedMemory.sharedMemoryAddresses[sharedMemory.length++] = memoryAddress; // Add the new element
	return 0; // Success   	
}

int initSharedMemory(struct sharedMem sharedMemory, int size) {

	sharedMemory.id = shmget(IPC_PRIVATE, size, IPC_CREAT|S_IRUSR|S_IWUSR);

	if(sharedMemory.id < 0) {

		perror ("Unable to obtain shared memory\n");

		exit (1);

	}

	sharedMemory.address = shmat(sharedMemory.id, 0, 0);

	if(sharedMemory.address == (void*) -1) {

		perror ("Unable to attach\n");

		exit (1);

	}

	insertIntoSharedMemArray(sharedMemory);

	return 0;
}

int cleanupSharedMemoryAddress(struct sharedMem *sharedMemory) {
	if(shmdt(sharedMemory->address) < 0) {

		perror ("Unable to detach\n");

		exit (1);

	}

	if(shmctl(sharedMemory->id, IPC_RMID, 0) < 0) {

		perror ("Unable to deallocate\n");

		exit(1);

	}

	return 0;
}

int initSemaphore(int resource, int resourceCount) {

 	int semId = semget (IPC_PRIVATE, 1, 00600);

	if(semId == -1) {
		perror("semget failed");
		exit(1);
	}

        int ctlErrorCode = semctl (semId, 0, SETVAL, resourceCount);
	
	if(ctlErrorCode == -1) {
		perror("semctl failed");
		exit(1);
	}
	
	insertIntoSemaphoreArray(resource, semId);

	return semId;
}

int cleanupSemaphore(int semId) {
	if(semctl (semId, 0, IPC_RMID) == -1) {
		perror("Failed to remove semaphores");
	} 
	//TODO: Add erorr checking
	return 0;
}

int cleanupSemaphores() {

	int length = semaphores.length;
	for(int i = 0; i < length; i++) {
		cleanupSemaphore(semaphores.semaphoreIds[i]);
	}

	free(semaphores.semaphoreIds);
	return 0;

}

int cleanupSharedMemory() {
	
	int length = sharedMemory.length;
	for(int i = 0; i < length; i++) {
		
		struct sharedMem *memoryContainer = &(sharedMemory.sharedMemoryAddresses[i]);
		cleanupSharedMemoryAddress(memoryContainer);
	}
	
	free(sharedMemory.sharedMemoryAddresses);
	return 0;

}

int getSemIdFromResource(int resource) {
	return semaphores.semaphoreIds[resource];
}

int useResource(int resource) {
 
	int semId = getSemIdFromResource(resource);

	struct sembuf sbuf;
	sbuf.sem_num = 0;
	sbuf.sem_op = -1;
	sbuf.sem_flg = SEM_UNDO;			

	semop (semId, &sbuf, 1);
	//TODO: Perform error checking...
	//TODO: Perform critical section here...
	
	sbuf.sem_op = 1;
	semop (semId, &sbuf, 1);
	
	return 0;		
}

int isPantryItem(int item) {
	int pantrySize = sizeof(pantryIngredients) / sizeof(pantryIngredients[0]);

	for(int i = 0; i < pantrySize; i++) {
		if(pantryIngredients[i] == item) {
			return 1;
		}
	}

	return 0;
}

int isRefrigeratorItem(int item) {
	int refrigeratorSize = sizeof(refrigeratorIngredients) / sizeof(refrigeratorIngredients[0]);
	
	for(int i = 0; i < refrigeratorSize; i++) {
		if(refrigeratorIngredients[i] == item) {
			return 1;
		}
	}

	return 0;
}


void sigHandler(int signal) {
	if (signal == SIGINT) {

		printf("\nTerminating program...\n");
		
		cleanupSemaphores();
		cleanupSharedMemory();
		exit(0);

	}

}

int main() {
	signal(SIGINT, sigHandler);

	int mixerId = initSemaphore(MIXER, 2);
	int pantryId = initSemaphore(PANTRY, 1);
	int refrigeratorId = initSemaphore(REFRIGERATOR, 2);
	int bowlId = initSemaphore(BOWL, 3);
	int spoonId = initSemaphore(SPOON, 5);
	int ovenId = initSemaphore(OVEN, 1);

	int bakers = -1;

	while(bakers < 0) {
                printf("How many bakers would you like\n");

                scanf("%d", &bakers);
        	if(bakers < 0) {
                        printf("Please provide a valid number of bakers.");
 		}
	}

	//Create n threads, with each one representing a baker.
	//struct sharedMem mem;
	//initSharedMemory(mem, sizeof(char));

	while(1) {
		//Busy waiting. Makes only way for program to end via termination
	}

	printf("Program finished");

	return 0;

}
