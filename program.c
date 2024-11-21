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

int semOffset = OVEN + 1;

int flourSemId;
int sugarSemId;
int yeastSemId;
int bakingSodaSemId;
int saltSemId;
int cinnamonSemId;
int eggsSemId;
int milkSemId;
int butterSemId;

const char *red = "\x1b[31m";
const char *green = "\x1b[32m";
const char *yellow = "\x1b[33m";
const char *blue = "\x1b[34m";
const char *magenta = "\x1b[35m";
const char *cyan = "\x1b[36m";
const char *reset_color = "\x1b[0m";

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

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

typedef struct {
	int ingredients[9];
} Refrigerator;

void initializeRefrigerator(Refrigerator* fridge) {
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

typedef struct {
	int ingredients[9];
} Pantry;

void initializePantry(Pantry* pantry) {
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

int isIn(const int haystack[], const int haystackSize, const int needle) {

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

int initSharedMemory(struct sharedMem sharedMemory, key_t key, int size) {

        sharedMemory.id = shmget(key, size, IPC_CREAT | S_IRUSR | S_IWUSR);

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

int decSem(int semId) {

	struct sembuf sbuf;
	sbuf.sem_num = 0;
	sbuf.sem_op = -1;
	sbuf.sem_flg = SEM_UNDO;

	if (semop(semId, &sbuf, 1) == -1) {
		perror("Unable to use resource");
	}

	return 0;
}

int incSem(int semId) {
	struct sembuf sbuf;
	sbuf.sem_num = 0;
	sbuf.sem_op = 1;
	sbuf.sem_flg = SEM_UNDO;

	if (semop(semId, &sbuf, 1) == -1) {
		perror("Unable to use resource");
	}

	return 0;
}

int useResource(int resource) {

	int semId = getSemIdFromResource(resource);

	return decSem(semId);

}
int useIngredient(int ingredient) {
	int semId = getSemIdFromResource(semOffset + ingredient);

	return decSem(semId);
}

int recoverResource(int resource) {

	int semId = getSemIdFromResource(resource);

	return incSem(semId);
}

int recoverIngredient(int ingredient) {
	int semId = getSemIdFromResource(semOffset + ingredient);

	return incSem(semId);
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

void decSemaphores(int bakerId, int ingredient) {
	if (isIn(pantryIngredients, 6, ingredient)) {
		printf("Baker %d is looking to enter the pantry\n", bakerId);
		useResource(PANTRY);
		printf("Baker %d entered the pantry\n", bakerId);
	}

	if (isIn(refrigeratorIngredients, 3, ingredient)) {
		printf("Baker %d is looking to enter the refrigerator\n", bakerId);
		useResource(REFRIGERATOR);
		printf("Baker %d entered the refrigerator\n", bakerId);
	}

	printf("Baker %d is waiting for ingredient %d\n", bakerId, ingredient);

	useIngredient(ingredient);

}

void incIngredientSemaphores(int bakerId, int ingredient) {
	if (isIn(pantryIngredients, 6, ingredient)) {
		printf("Baker %d is looking to leave the pantry\n", bakerId);
		recoverResource(PANTRY);
		printf("Baker %d left the pantry\n", bakerId);
	}

	if (isIn(refrigeratorIngredients, 3, ingredient)) {
		printf("Baker %d is looking to leave the refrigerator\n", bakerId);
		recoverResource(REFRIGERATOR);
		printf("Baker %d left the refrigerator\n", bakerId);
	}

	recoverIngredient(ingredient);

}

int* initRecipes(int recipe, int initRecipe[]) {

	printf("Initializing %d recipe\n", recipe);
	if (recipe < 0 || recipe > 4) {
		perror("Not a valid recipe");
		exit(1);
	}
	else {
		if (recipe == COOKIE) {
			printf("Initting a cookie\n");
			initRecipe[FLOUR] = 1;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 0;
			initRecipe[BAKING_SODA] = 0;
			initRecipe[SALT] = 0;
			initRecipe[CINNAMON] = 0;
			initRecipe[EGGS] = 0;
			initRecipe[MILK] = 1;
			initRecipe[BUTTER] = 1;
		}
		if (recipe == PANCAKE) {
			initRecipe[FLOUR] = 1;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 0;
			initRecipe[BAKING_SODA] = 1;
			initRecipe[SALT] = 1;
			initRecipe[CINNAMON] = 0;
			initRecipe[EGGS] = 1;
			initRecipe[MILK] = 1;
			initRecipe[BUTTER] = 1;
		}
		if (recipe == PIZZA) {
			initRecipe[FLOUR] = 0;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 1;
			initRecipe[BAKING_SODA] = 0;
			initRecipe[SALT] = 1;
			initRecipe[CINNAMON] = 0;
			initRecipe[EGGS] = 0;
			initRecipe[MILK] = 0;
			initRecipe[BUTTER] = 0;
		}
		if (recipe == PRETZEL) {
			initRecipe[FLOUR] = 1;
			initRecipe[SUGAR] = 1;
			initRecipe[YEAST] = 1;
			initRecipe[BAKING_SODA] = 1;
			initRecipe[SALT] = 1;
			initRecipe[CINNAMON] = 0;
			initRecipe[EGGS] = 1;
			initRecipe[MILK] = 0;
			initRecipe[BUTTER] = 0;
		}
		if (recipe == CINROLL) {
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

		printf("Finished initializing %d recipe\n", recipe);
		return initRecipe;
	}
}


int checkRecipe(int recipe[], int size) {
	if (size != 9) {
		perror("Not a valid recipe");
		exit(1);
	}
	else {
		for (int i = 0; i < size; i++) {
			if (recipe[i] == 1) {
				return 1;
			}
		}
	}

	return 0;
}

void addIngredient(int* recipe, int ingredient) {

	if (recipe[ingredient] == 1) {
		recipe[ingredient] = 0;
	}
}

int getIngredient(int bakerId, int* recipe, int ingredient) {
	decSemaphores(bakerId, ingredient);
	addIngredient(recipe, ingredient);
	printf("Baker %d got ingredient %d\n", bakerId, ingredient);

	sleep(1);

	printf("Baker %d is returning ingredient %d\n", bakerId, ingredient);

	incIngredientSemaphores(bakerId, ingredient);

	return 1;
}

//Return: Success of getting an ingredient
int checkIngredient(int bakerId, int* recipe, int size, int ingredient) {

	if (size != 9) {
		perror("Not a valid recipe");
		exit(1);
	}
	if (ingredient > 8 || ingredient < 0) {
		perror("Ingredient is not a valid ingredient");
	}
	else {
		if (recipe[ingredient] == 0) {
			return 0;
		}

		return getIngredient(bakerId, recipe, ingredient);

	}

	return 0;
}

// Return: If an ingredient was successfully gotten
int getAvailableIngredients(int bakerId, int* recipe) {
	int updated = 0;
	for (int i = 0; i < 9; i++) {
		updated |= checkIngredient(bakerId, recipe, 9, i);
	}

	return updated;
}

int isARecipeRemaining(int recipes[], int length) {
	for (int i = 0; i < length; i++) {
		if (recipes[i]) {
			return 1;
		}
	}

	return 0;
}

int getMixingResources(int bakerId, int* tools, int size) {
	if (size != 3) {
		perror("Not a valid set of tools");
	}

	printf("Baker %d is looking to aquire a mixer\n", bakerId);
	tools[MIXER] = useResource(MIXER);
	printf("Baker %d aquired a mixer\n", bakerId);

	printf("Baker %d is looking to aquire a bowl\n", bakerId);
	tools[MIXER] = useResource(BOWL);
	printf("Baker %d aquired a bowl\n", bakerId);

	printf("Baker %d is looking to aquire as spoon\n", bakerId);
	tools[MIXER] = useResource(SPOON);
	printf("Baker %d aquired a spoon\n", bakerId);

	return 0;
}

int returnMixingResources(int bakerId) {
	recoverResource(MIXER);
	recoverResource(BOWL);
	recoverResource(SPOON);

	return 0;
}

int mixIngredients(int bakerId, int* tools, int size) {
	getMixingResources(bakerId, tools, size);

	printf("Baker %d is mixing the ingredients together\n", bakerId);

	sleep(1);

	printf("Baker %d mixed all of the ingredients together\n", bakerId);

	returnMixingResources(bakerId);

	return 0;
}


int cookRecipe(int bakerId, int recipe) {
	printf("Baker %d is looking to use the oven to cook recipe %d\n", bakerId, recipe);

	useResource(OVEN);

	printf("Baker %d is using the oven to cook recipe %d\n", bakerId, recipe);

	sleep(3);

	printf("Baker %d finished using the oven to cook recipe %d\n", bakerId, recipe);

	recoverResource(OVEN);

	return 0;
}

void* simulateBaker(void* val) {
	//Put all baker logic in here
	//NOTE: When this function terminates, it will reclaim memory from the thread
	int* bakerIdRef = (int*)val;
	int bakerId = *bakerIdRef;

	//Setup recipes
	//TODO: Fix init recipe array
	int cookieArray[9] = {};
	int pancakeArray[9];
	int pizzaDoughArray[9];
	int softPretzelArray[9];
	int cinnamonRollArray[9];

	int* cookie = initRecipes(COOKIE, cookieArray);
	int* pancake = initRecipes(PANCAKE, pancakeArray);
	int* pizzaDough = initRecipes(PIZZA, pizzaDoughArray);
	int* softPretzel = initRecipes(PRETZEL, softPretzelArray);
	int* cinnamonRoll = initRecipes(CINROLL, cinnamonRollArray);

	int recipesRemaining[] = { 1, 1, 1, 1, 1 };

	//Setup tools
	int tools[3];
	tools[MIXER] = 1;
	tools[BOWL] = 1;
	tools[SPOON] = 1;

	//Iterate through each of the recipes.
	int i = 0;

	while (isARecipeRemaining(recipesRemaining, 5)) {

		if (!recipesRemaining[i]) {
			i++;
			i = i % 5;
			continue;
		}

		int* currentRecipe = NULL;
		if (i == COOKIE) {
			currentRecipe = cookie;
			printf("Baker %d is working on making Cookies\n", bakerId);
		}

		if (i == PANCAKE) {
			currentRecipe = pancake;
			printf("Baker %d is working on making Pancakes\n", bakerId);
		}

		if (i == PIZZA) {
			currentRecipe = pizzaDough;
			printf("Baker %d is working on making Pizza Dough\n", bakerId);
		}

		if (i == PRETZEL) {
			currentRecipe = softPretzel;
			printf("Baker %d is working on making Soft Pretzels\n", bakerId);
		}

		if (i == CINROLL) {
			currentRecipe = cinnamonRoll;
			printf("Baker %d is working on making Cinnamon Rolls\n", bakerId);
		}

		if (currentRecipe == NULL) {
			printf("Invalid index for recipe found within simulateBaker\n");
		}

		int isRecipeComplete = getAvailableIngredients(bakerId, currentRecipe);
		recipesRemaining[i] = !isRecipeComplete;

		if (isRecipeComplete) {
			mixIngredients(bakerId, tools, 3);

			cookRecipe(bakerId, i);

			printf("Baker %d finished recipe %d\n", bakerId, i);

		}

		i++;
		i = i % 5;

	}

	printf("Baker %d has finished\n", bakerId);

	free(bakerIdRef);
	return NULL;
}

void spawnThread(int bakerId) {
	pthread_t thread;

	int* id = malloc(sizeof(int));
	*id = bakerId;

	printf("Initializing baker %d\n", *id);

	int threadStatus = pthread_create(&thread, NULL, simulateBaker, id);

	if (threadStatus != 0) {
		fprintf(stderr, "Thread create error %d: %s\n", threadStatus, strerror(threadStatus));

		exit(1);
	}

	pthread_detach(thread);
}

void spawnThreads(int n) {
	printf("Initializing %d bakers\n", n);
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

	flourSemId = initSemaphore(semOffset + FLOUR, 1);
	sugarSemId = initSemaphore(semOffset + SUGAR, 1);
	yeastSemId = initSemaphore(semOffset + YEAST, 1);
	bakingSodaSemId = initSemaphore(semOffset + BAKING_SODA, 1);
	saltSemId = initSemaphore(semOffset + SALT, 1);
	cinnamonSemId = initSemaphore(semOffset + CINNAMON, 1);
	eggsSemId = initSemaphore(semOffset + EGGS, 2);
	milkSemId = initSemaphore(semOffset + MILK, 2);
	butterSemId = initSemaphore(semOffset + BUTTER, 2);

	//Write to shared memory the baker and the recipe to get ramsied.
	struct sharedMem sharedMemory;
	key_t key = ftok("./program.c", 0);

	initSharedMemory(sharedMemory, key, 2 * sizeof(int));


	while(1) {
		int bakers = -1;

		while (bakers < 0) {
			printf("How many bakers would you like\n");

			scanf("%d", &bakers);
			if (bakers < 0) {
				printf("Please provide a valid number of bakers.");
			}
		}

		//Create n threads, with each one representing a baker.
		srand(time(NULL));

		sharedMemory[0] = rand % bakers; 
		sharedMemory[1] = rand % 5;

		spawnThreads(bakers);



		//Busy waiting. Makes the only way for program to end via termination
	}

	printf("Program finished");

	return 0;

}
