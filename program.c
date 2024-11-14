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
const int BOWL = 1;
const int SPOON = 2;

const int PANTRY = 3;
const int REFRIGERATOR = 4;

const int OVEN = 5;

const int COOKIE = 0;
const int PANCAKE = 1;
const int PIZZA = 2;
const int PRETZEL = 3;
const int CINROLL = 4;

const int cookieRecipe[] = { FLOUR, SUGAR, MILK, BUTTER };
const int pancakeRecipe[] = { FLOUR, SUGAR, BAKING_SODA, SALT, EGGS, MILK, BUTTER };
const int pizzaDoughRecipe[] = { YEAST, SUGAR, SALT };
const int softPretzelRecipe[] = { FLOUR, SUGAR, SALT, YEAST, BAKING_SODA, EGGS };
const int cinnamonRollRecipe[] = { FLOUR, SUGAR, SALT, BUTTER, EGGS, CINNAMON };

const int pantryIngredients[] = { FLOUR, SUGAR, YEAST, BAKING_SODA, SALT, CINNAMON };
const int refrigeratorIngredients[] = { EGGS, MILK, BUTTER };

int mixerSemID;
int pantrySemID;
int refrigeratorSemID;
int bowlSemID;
int spoonSemID;
int ovenSemID;

struct semaphoresStruct {
	int length;
	int* semaphoreIds;
};

struct sharedMem {
	int id;
	long int* address;
};

struct sharedMemStruct {
	int length;
	struct sharedMem* sharedMemoryAddresses;
};

struct semaphoresStruct semaphores;

struct sharedMemStruct sharedMemory;

typedef struct{
    int ingredients[9];
} Refrigerator;

void initializeRefrigerator(Refrigerator *fridge) {
    fridge->ingredients[FLOUR] = 0;
    fridge->ingredients[SUGAR] = 0;
    fridge->ingredients[YEAST] = 0;
    fridge->ingredients[BAKING_SODA] = 0;
    fridge->ingredients[SALT] = 0;
    fridge->ingredients[CINNAMON] = 0;
    fridge->ingredients[EGGS] = 1;
    fridge->ingredients[MILK] = 1;
    fridge->ingredients[BUTTER] = 1;
}

typedef struct{
    int ingredients[9];
} Pantry;

void initializePantry(Pantry *pantry) {
    pantry->ingredients[FLOUR] = 1;
    pantry->ingredients[SUGAR] = 1;
    pantry->ingredients[YEAST] = 1;
    pantry->ingredients[BAKING_SODA] = 1;
    pantry->ingredients[SALT] = 1;
    pantry->ingredients[CINNAMON] = 1;
    pantry->ingredients[EGGS] = 0;
    pantry->ingredients[MILK] = 0;
    pantry->ingredients[BUTTER] = 0;
}

int isIn(int haystack[], int haystackSize, int needle) {

	for (int i = 0; i < haystackSize; i++) {
		if (haystack[i] == needle) {
			return 1;
		}
	}

	return 0;
}

int insertIntoSemaphoreArray(int resource, int semaphoreId) {
	if (resource >= semaphores.length) {
		int* temp = realloc(semaphores.semaphoreIds, (resource + 1) * sizeof(int));

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
	struct sharedMem* temp = realloc(sharedMemory.sharedMemoryAddresses,
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

	sharedMemory.id = shmget(IPC_PRIVATE, size, IPC_CREAT | S_IRUSR | S_IWUSR);

	if (sharedMemory.id < 0) {

		perror("Unable to obtain shared memory\n");

		exit(1);

	}

	sharedMemory.address = shmat(sharedMemory.id, 0, 0);

	if (sharedMemory.address == (void*)-1) {

		perror("Unable to attach\n");

		exit(1);

	}

	insertIntoSharedMemArray(sharedMemory);

	return 0;
}

int cleanupSharedMemoryAddress(struct sharedMem* sharedMemory) {
	if (shmdt(sharedMemory->address) < 0) {

		perror("Unable to detach\n");

		exit(1);

	}

	if (shmctl(sharedMemory->id, IPC_RMID, 0) < 0) {

		perror("Unable to deallocate\n");

		exit(1);

	}

	return 0;
}

int initSemaphore(int resource, int resourceCount) {

	int semId = semget(IPC_PRIVATE, 1, 00600);

	if (semId == -1) {
		perror("semget failed");
		exit(1);
	}

	int ctlErrorCode = semctl(semId, 0, SETVAL, resourceCount);

	if (ctlErrorCode == -1) {
		perror("semctl failed");
		exit(1);
	}

	insertIntoSemaphoreArray(resource, semId);

	return semId;
}

int cleanupSemaphore(int semId) {
	if (semctl(semId, 0, IPC_RMID) == -1) {
		perror("Failed to remove semaphores");
	}
	//TODO: Add erorr checking
	return 0;
}

int cleanupSemaphores() {

	int length = semaphores.length;
	for (int i = 0; i < length; i++) {
		cleanupSemaphore(semaphores.semaphoreIds[i]);
	}

	free(semaphores.semaphoreIds);
	return 0;

}

int cleanupSharedMemory() {

	int length = sharedMemory.length;
	for (int i = 0; i < length; i++) {

		struct sharedMem* memoryContainer = &(sharedMemory.sharedMemoryAddresses[i]);
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

	semop(semId, &sbuf, 1);
	//TODO: Perform error checking...
	//TODO: Perform critical section here...

	sbuf.sem_op = 1;
	semop(semId, &sbuf, 1);

	return 0;
}

int isPantryItem(int item) {
	int pantrySize = sizeof(pantryIngredients) / sizeof(pantryIngredients[0]);

	for (int i = 0; i < pantrySize; i++) {
		if (pantryIngredients[i] == item) {
			return 1;
		}
	}

	return 0;
}

int isRefrigeratorItem(int item) {
	int refrigeratorSize = sizeof(refrigeratorIngredients) / sizeof(refrigeratorIngredients[0]);

	for (int i = 0; i < refrigeratorSize; i++) {
		if (refrigeratorIngredients[i] == item) {
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

void decSemaphores(int resource) {
	if (isIn(pantryIngredients, 6, resource)) {
		useResource(pantrySemID);
	}

	if (isIn(refrigeratorIngredients, 3, resource)) {
		useResource(refrigeratorSemID);
	}

	//useResource(ingredientSemId);
	
}

int* initRecipes(int recipe) {
	static int initRecipe[9];

	printf("Initializing %d recipe\n", recipe);
	if (recipe < 0 || recipe > 4) {
		perror("Not a valid recipe");
		exit(1);
	}
	else {
		switch (recipe) {
		case COOKIE:
			initRecipe[FLOUR] = 1;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 0;
			initRecipe[BAKING_SODA] = 0;
			initRecipe[SALT] = 0;
			initRecipe[CINNAMON] = 0;
			initRecipe[EGGS] = 0;
			initRecipe[MILK] = 1;
			initRecipe[BUTTER] = 1;
			break;
		case PANCAKE:
			initRecipe[FLOUR] = 1;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 0;
			initRecipe[BAKING_SODA] = 1;
			initRecipe[SALT] = 1;
			initRecipe[CINNAMON] = 0;
			initRecipe[EGGS] = 1;
			initRecipe[MILK] = 1;
			initRecipe[BUTTER] = 1;
			break;
		case PIZZA:
			initRecipe[FLOUR] = 0;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 1;
			initRecipe[BAKING_SODA] = 0;
			initRecipe[SALT] = 1;
			initRecipe[CINNAMON] = 0;
			initRecipe[EGGS] = 0;
			initRecipe[MILK] = 0;
			initRecipe[BUTTER] = 0;
			break;
		case PRETZEL:
			initRecipe[FLOUR] = 1;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 1;
			initRecipe[BAKING_SODA] = 1;
			initRecipe[SALT] = 1;
			initRecipe[CINNAMON] = 0;
			initRecipe[EGGS] = 1;
			initRecipe[MILK] = 0;
			initRecipe[BUTTER] = 0;
			break;
		case CINROLL:
			initRecipe[FLOUR] = 1;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 0;
			initRecipe[BAKING_SODA] = 0;
			initRecipe[SALT] = 1;
			initRecipe[CINNAMON] = 1;
			initRecipe[EGGS] = 1;
			initRecipe[MILK] = 0;
			initRecipe[BUTTER] = 1;
		}

		printf("Finished %d recipe\n", recipe);
		return initRecipe;
	}
}


int checkRecipe(int recipe[]) {

	if (sizeof(&recipe) != 9) {
		perror("Not a valid recipe");
		exit(1);
	}
	else {
		for (int i = 0; i < 9; i++) {
			if (recipe[i] == 1) {
				return 1;
			}
		}
	}

	return 0;
}

//Return: Success of getting an ingredient
int checkIngredient(int* recipe, int ingredient) {

	if (sizeof(*recipe) / sizeof(recipe[0]) != 9) {
		perror("Not a valid recipe");
		exit(1);
	}
	if (ingredient > 8 || ingredient < 0) {
		perror("Ingredient is not a valid ingredient")
	}
	else {
		if (recipe[ingredient] == 1) {
			return 0;
		}

		return getIngredient(&recipe, ingredient);

	}

	return 0;
}

void addIngredient(int *recipe, int ingredient) {

	if (*recipe[ingredient] == 1) {
		*recipe[ingredient] = 0;
	}
}
// Return: If an ingredient was successfully gotten
int getAvailableIngredients(int* recipe) {
	int updated = 0
	for (int i = 0; i < 9; i++) {
		updated |= checkIngredient(recipe, i);
	}

	return updated;
}

int getIngredient(int *recipe, int ingredient) {

}

int isARecipeRemaining(int recipes[], int length) {
	for (int i = 0; i < length; i++){
		if (recipes[i]) {
			return 1;
		}
	}

	return 0;
}

void* simulateBaker(void* val) {
	//Put all baker logic in here
	//NOTE: When this function terminates, it will reclaim memory from the thread
	int* bakerId = (int*)val;

	//Setup recipes
	int* cookie = initRecipes(COOKIE);
	int* pancake = initRecipes(PANCAKE);
	int* pizzaDough = initRecipes(PIZZA);
	int* softPretzel = initRecipes(PRETZEL);
	int* cinnamonRoll = initRecipes(CINROLL);

	int recipesRemaining[] = {1, 1, 1, 1, 1};

	//Setup tools
	int tools[3];
	tools[MIXER] = 1;
	tools[BOWL] = 1;
	tools[SPOON] = 1;

	//Iterate through each of the recipes.
	int i = 0;

	while (isARecipeRemaining(recipesRemaining, 5)) {
		i++;
		i = i % 5;

		if (!recipesRemaining[i]) {
			continue;
		}

		int* currentRecipe;
		switch (i) {
			case 0:
				currentRecipe = cookie;
				break;
			case 1:
				currentRecipe = pancake;
				break;
			case 2:
				currentRecipe = pizzaDough;
				break;
			case 3:
				currentRecipe = softPretzel;
				break;
			case 4:
				currentRecipe = cinnamonRoll;
				break;

			default:
				printf("Invalid index for recipe found within simulateBaker.");
		}

		int isRecipeComplete = getAvailableIngredients(currentRecipe);

		if (isRecipeComplete) {
			//Handle mixers...
			
			//Handle semaphores. 

		}
	}

	free(bakerId);
	printf("Baker has finished\n");
	return NULL;
}

void spawnThread(int bakerId) {
	pthread_t thread;

	int* id = malloc(sizeof(int));
	*id = bakerId;

	int threadStatus = pthread_create(&thread, NULL, simulateBaker, id);

	if (threadStatus != 0) {
		fprintf(stderr, "Thread create error %d: %s\n", threadStatus, strerror(threadStatus));

		exit(1);
	}

	pthread_detach(thread);
}

void spawnThreads(int n) {

	//for(int bakerId = 1; bakerId <= n; bakerId++) {
	for (int bakerId = 0; bakerId < n; bakerId++) {
		spawnThread(bakerId);
	}

}

int main() {
	signal(SIGINT, sigHandler);

	mixerSemID = initSemaphore(MIXER, 2);
	pantrySemID = initSemaphore(PANTRY, 1);
	refrigeratorSemID = initSemaphore(REFRIGERATOR, 2);
	bowlSemID = initSemaphore(BOWL, 3);
	spoonSemID = initSemaphore(SPOON, 5);
	ovenSemID = initSemaphore(OVEN, 1);

	int bakers = -1;


	while (bakers < 0) {
		printf("How many bakers would you like\n");

		scanf("%d", &bakers);
		if (bakers < 0) {
			printf("Please provide a valid number of bakers.");
		}
	}

	//Create n threads, with each one representing a baker.
	//struct sharedMem mem;
	//initSharedMemory(mem, sizeof(char));
	spawnThreads(bakers);


	while (1) {
		//Busy waiting. Makes the only way for program to end via termination
	}

	printf("Program finished");

	return 0;

}
