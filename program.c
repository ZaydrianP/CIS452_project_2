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

const char* colors[] = {
	"\033[0;31m", // Red
	"\033[0;32m", // Green
	"\033[0;33m", // Yellow
	"\033[0;34m", // Blue
	"\033[0;35m", // Magenta
	"\033[0;36m", // Cyan
	"\033[0;37m"  // White
};

const char* programPath = "./program.c";
int ramsiedSharedMemoryID = 0;

/**
 * @struct semaphoresStruct
 * @brief A structure to hold semaphore information.
 *
 * This structure contains information about semaphores, including the number
 * of semaphores and an array of semaphore IDs.
 *
 * @var semaphoresStruct::length
 * The number of semaphores.
 *
 * @var semaphoresStruct::semaphoreIds
 * A pointer to an array of semaphore IDs.
 */
struct semaphoresStruct {
	int length;
	int* semaphoreIds;
};

/**
 * struct sharedMem - Structure to represent shared memory.
 * @id: An integer representing the identifier for the shared memory.
 * @address: A pointer to a long integer representing the address of the shared memory.
 */
struct sharedMem {
	int id;
	long int* address;
};

/**
 * struct sharedMemStruct - A structure to hold shared memory information.
 * @length: The number of shared memory addresses.
 * @sharedMemoryAddresses: A pointer to an array of shared memory addresses.
 */
struct sharedMemStruct {
	int length;
	struct sharedMem* sharedMemoryAddresses;
};

struct semaphoresStruct semaphores;

struct sharedMemStruct sharedMemory;

/**
 * @brief A structure representing a refrigerator.
 *
 * This structure contains an array of integers representing the ingredients
 * stored in the refrigerator. Each element in the array corresponds to a
 * specific ingredient.
 */
typedef struct {
	int ingredients[9];
} Refrigerator;

/**
 * @brief Initializes the ingredients in the refrigerator to their default values.
 *
 * This function sets the quantities of various ingredients in the refrigerator to their initial values.
 *
 * @param fridge A pointer to the Refrigerator structure to be initialized.
 */
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

/**
 * @brief A structure representing a pantry with a fixed number of ingredients.
 *
 * This structure is used to store the quantities of up to 9 different ingredients.
 */
typedef struct {
	int ingredients[9];
} Pantry;

/**
 * @brief Initializes the pantry with default ingredient quantities.
 *
 * This function sets the initial quantities of various ingredients in the pantry.
 *
 * @param pantry A pointer to the Pantry structure to be initialized.
 */
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

/**
 * Checks if a given integer is present in an array.
 *
 * @param haystack An array of integers to search within.
 * @param haystackSize The size of the haystack array.
 * @param needle The integer value to search for in the haystack array.
 * @return Returns 1 if the needle is found in the haystack array, otherwise returns 0.
 */
int isIn(const int haystack[], const int haystackSize, const int needle) {

	for (int i = 0; i < haystackSize; i++) {
		if (haystack[i] == needle) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Inserts a semaphore ID into the semaphore array at the specified resource index.
 *
 * This function ensures that the semaphore array is large enough to accommodate the specified
 * resource index. If the array is not large enough, it reallocates memory to expand the array.
 * If memory allocation fails, it returns an error code.
 *
 * @param resource The index in the semaphore array where the semaphore ID should be inserted.
 * @param semaphoreId The semaphore ID to be inserted into the array.
 * @return int Returns 0 on success, or -1 if memory allocation fails.
 */
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
/**
 * @brief Inserts a new element into the shared memory array.
 *
 * This function reallocates memory for the shared memory array to accommodate
 * a new element, then inserts the provided element into the array.
 *
 * @param memoryAddress Pointer to the sharedMem structure to be inserted.
 * @return int Returns 0 on success, or -1 if memory allocation fails.
 */
int insertIntoSharedMemArray(struct sharedMem* memoryAddress) {

	struct sharedMem* temp = realloc(sharedMemory.sharedMemoryAddresses,

		(sharedMemory.length + 1) * sizeof(struct sharedMem));



	if (temp == NULL) {

		perror("Failed to allocate memory for sharedMemoryAddresses");

		return -1; // Memory allocation failure

	}



	sharedMemory.sharedMemoryAddresses = temp;

	sharedMemory.sharedMemoryAddresses[sharedMemory.length++] = *memoryAddress; // Add the new element

	return 0; // Success

}

/**
 * @brief Initializes shared memory segment.
 *
 * This function creates a shared memory segment with the specified key and size,
 * attaches it to the process's address space, and inserts it into the shared memory array.
 *
 * @param sharedMemory Pointer to the sharedMem structure to be initialized.
 * @param key The key to be used for the shared memory segment.
 * @param size The size of the shared memory segment.
 * @param isAlreadyCreated Boolean indicating whether the sharedMemory already exists
 * @return Returns 0 on success, exits the program on failure.
 */
int initSharedMemory(struct sharedMem* sharedMemory, key_t key, int size, int isAlreadyCreated) {

	sharedMemory->id = shmget(key, size, IPC_CREAT | S_IRUSR | S_IWUSR);

	if (sharedMemory->id < 0) {

		perror("Unable to obtain shared memory\n");

		exit(1);

	}

	sharedMemory->address = shmat(sharedMemory->id, 0, 0);

	if (sharedMemory->address == (void*)-1) {

		perror("Unable to attach\n");

		exit(1);

	}

	if (!isAlreadyCreated) {
		insertIntoSharedMemArray(sharedMemory);
	}

	return 0;
}

/**
 * @brief Cleans up the shared memory by detaching and deallocating it.
 *
 * This function detaches the shared memory segment from the address space of the calling process
 * and then deallocates the shared memory segment. If either operation fails, an error message is
 * printed and the program exits with a status of 1.
 *
 * @param sharedMemory A pointer to the sharedMem structure that contains the shared memory segment's
 *                     address and identifier.
 * @return Returns 0 on successful cleanup of the shared memory.
 */
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

/**
 * Initializes a semaphore with a given resource count.
 *
 * This function creates a new semaphore, sets its initial value, and inserts it into a semaphore array.
 *
 * @param resource The resource identifier to associate with the semaphore.
 * @param resourceCount The initial count/value for the semaphore.
 * @return The semaphore ID on success.
 * @retval -1 on failure to create or initialize the semaphore.
 */
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

/**
 * @brief Cleans up and removes a semaphore set.
 *
 * This function attempts to remove a semaphore set identified by the given semaphore ID.
 * If the removal fails, an error message is printed to standard error.
 *
 * @param semId The identifier of the semaphore set to be removed.
 * @return Returns 0 on success. Note: Error checking and handling need to be added.
 */
int cleanupSemaphore(int semId) {
	if (semctl(semId, 0, IPC_RMID) == -1) {
		perror("Failed to remove semaphores");
	}
	//TODO: Add error checking
	return 0;
}

/**
 * @brief Cleans up all semaphores by releasing their resources.
 *
 * This function iterates through all semaphore IDs stored in the `semaphores` structure,
 * calls `cleanupSemaphore` on each one to release its resources, and then frees the memory
 * allocated for the semaphore IDs array.
 *
 * @return Returns 0 upon successful cleanup.
 */
int cleanupSemaphores() {

	int length = semaphores.length;
	for (int i = 0; i < length; i++) {
		cleanupSemaphore(semaphores.semaphoreIds[i]);
	}

	free(semaphores.semaphoreIds);
	return 0;

}

/**
 * @brief Cleans up shared memory by iterating through each shared memory address
 *        and freeing the allocated memory.
 *
 * This function iterates through the shared memory addresses stored in the
 * sharedMemory structure, calls the cleanupSharedMemoryAddress function for
 * each address, and then frees the memory allocated for the shared memory addresses.
 *
 * @return Returns 0 upon successful cleanup.
 */
int cleanupSharedMemory() {

	int length = sharedMemory.length;
	for (int i = 0; i < length; i++) {

		struct sharedMem* memoryContainer = &(sharedMemory.sharedMemoryAddresses[i]);
		cleanupSharedMemoryAddress(memoryContainer);
	}

	free(sharedMemory.sharedMemoryAddresses);
	return 0;

}

/**
 * @brief Retrieves the semaphore ID associated with a given resource.
 *
 * This function takes a resource identifier as input and returns the
 * corresponding semaphore ID from the semaphores structure.
 *
 * @param resource The identifier of the resource for which the semaphore ID is needed.
 * @return The semaphore ID associated with the specified resource.
 */
int getSemIdFromResource(int resource) {
	return semaphores.semaphoreIds[resource];
}

/**
 * @brief Decrements the semaphore value.
 *
 * This function performs a semaphore operation to decrement the semaphore
 * value by 1. It uses the `semop` system call to perform the operation.
 *
 * @param semId The semaphore ID.
 * @return Returns 0 on success, or prints an error message if the operation fails.
 */
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

/**
 * @brief Increment a semaphore.
 *
 * This function increments the semaphore specified by the semaphore ID (semId).
 * It uses the semop system call to perform the increment operation.
 *
 * @param semId The ID of the semaphore to increment.
 * @return Returns 0 on success. If the semop call fails, an error message is printed.
 */
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

/**
 * @brief Uses a resource by decrementing its associated semaphore.
 *
 * This function retrieves the semaphore ID associated with the given resource
 * and then decrements the semaphore to indicate usage of the resource.
 *
 * @param resource The identifier of the resource to be used.
 * @return int The result of the semaphore decrement operation.
 */
int useResource(int resource) {

	int semId = getSemIdFromResource(resource);

	return decSem(semId);

}
/**
 * @brief Decrements the semaphore associated with the given ingredient.
 *
 * This function retrieves the semaphore ID for the specified ingredient
 * by adding the ingredient value to a predefined semaphore offset and
 * then calls the decSem function to decrement the semaphore.
 *
 * @param ingredient The ingredient identifier for which the semaphore
 *                   needs to be decremented.
 * @return int The result of the decSem function, typically indicating
 *             success or failure of the semaphore operation.
 */
int useIngredient(int ingredient) {
	int semId = getSemIdFromResource(semOffset + ingredient);

	return decSem(semId);
}

/**
 * @brief Recovers a resource by incrementing its associated semaphore.
 *
 * This function takes a resource identifier, retrieves the corresponding
 * semaphore ID using the getSemIdFromResource function, and then increments
 * the semaphore using the incSem function.
 *
 * @param resource The identifier of the resource to be recovered.
 * @return int The result of the incSem function, typically indicating success or failure.
 */
int recoverResource(int resource) {

	int semId = getSemIdFromResource(resource);

	return incSem(semId);
}

/**
 * @brief Recovers the specified ingredient by incrementing its semaphore.
 *
 * This function retrieves the semaphore ID associated with the given ingredient
 * and increments the semaphore to indicate that the ingredient has been recovered.
 *
 * @param ingredient The identifier of the ingredient to be recovered.
 * @return int The result of the semaphore increment operation.
 */
int recoverIngredient(int ingredient) {
	int semId = getSemIdFromResource(semOffset + ingredient);

	return incSem(semId);
}

/**
 * @brief Checks if a given item is in the pantry.
 *
 * This function iterates through the pantryIngredients array to determine
 * if the specified item is present in the pantry.
 *
 * @param item The item to check for in the pantry.
 * @return int Returns 1 if the item is found in the pantry, otherwise returns 0.
 */
int isPantryItem(int item) {
	int pantrySize = sizeof(pantryIngredients) / sizeof(pantryIngredients[0]);

	for (int i = 0; i < pantrySize; i++) {
		if (pantryIngredients[i] == item) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Checks if a given item is in the refrigerator.
 *
 * This function iterates through the refrigeratorIngredients array to determine
 * if the specified item is present. If the item is found, the function returns 1,
 * otherwise it returns 0.
 *
 * @param item The item to check for in the refrigerator.
 * @return int Returns 1 if the item is found in the refrigerator, otherwise returns 0.
 */
int isRefrigeratorItem(int item) {
	int refrigeratorSize = sizeof(refrigeratorIngredients) / sizeof(refrigeratorIngredients[0]);

	for (int i = 0; i < refrigeratorSize; i++) {
		if (refrigeratorIngredients[i] == item) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Signal handler for SIGINT.
 *
 * This function is called when the program receives a SIGINT signal (usually generated by pressing Ctrl+C).
 * It performs cleanup operations and terminates the program gracefully.
 *
 * @param signal The signal number received (expected to be SIGINT).
 */
void sigHandler(int signal) {
	if (signal == SIGINT) {

		printf("\nTerminating program...\n");

		cleanupSemaphores();
		cleanupSharedMemory();
		exit(0);

	}

}

/**
 * @brief Decrements the semaphores for the specified ingredient and baker.
 *
 * This function checks if the specified ingredient is in the pantry or refrigerator.
 * If the ingredient is in the pantry, the baker will attempt to enter the pantry and use the resource.
 * If the ingredient is in the refrigerator, the baker will attempt to enter the refrigerator and use the resource.
 * The function then waits for the specified ingredient to be available and uses it.
 *
 * @param bakerId The ID of the baker attempting to use the ingredient.
 * @param ingredient The ingredient that the baker needs.
 * @param color The color code for printing messages.
 * @param resetColor The color code to reset the terminal color.
 */
void decSemaphores(int bakerId, int ingredient, char* color, char* resetColor) {
	if (isIn(pantryIngredients, 6, ingredient)) {
		printf("%sBaker %d is looking to enter the pantry\n%s", color, bakerId, resetColor);
		useResource(PANTRY);
		printf("%sBaker %d entered the pantry\n%s", color, bakerId, resetColor);
	}

	if (isIn(refrigeratorIngredients, 3, ingredient)) {
		printf("%sBaker %d is looking to enter the refrigerator\n%s", color, bakerId, resetColor);
		useResource(REFRIGERATOR);
		printf("%sBaker %d entered the refrigerator\n%s", color, bakerId, resetColor);
	}

	printf("%sBaker %d is waiting for ingredient %d\n%s", color, bakerId, ingredient, resetColor);

	useIngredient(ingredient);

}

/**
 * @brief Increments the semaphores for the specified ingredient and handles resource recovery.
 *
 * This function checks if the given ingredient is in the pantry or refrigerator.
 * If the ingredient is in the pantry, it logs the baker's intention to leave the pantry,
 * recovers the pantry resource, and logs that the baker has left the pantry.
 * Similarly, if the ingredient is in the refrigerator, it logs the baker's intention to leave the refrigerator,
 * recovers the refrigerator resource, and logs that the baker has left the refrigerator.
 * Finally, it recovers the specified ingredient.
 *
 * @param bakerId The ID of the baker attempting to increment the ingredient semaphore.
 * @param ingredient The ingredient for which the semaphore needs to be incremented.
 * @param color The color code for printing messages.
 * @param resetColor The color code to reset the terminal color.
 */
void incIngredientSemaphores(int bakerId, int ingredient, char* color, char* resetColor) {
	if (isIn(pantryIngredients, 6, ingredient)) {
		printf("%sBaker %d is looking to leave the pantry\n%s", color, bakerId, resetColor);
		recoverResource(PANTRY);
		printf("%sBaker %d left the pantry\n%s", color, bakerId, resetColor);
	}

	if (isIn(refrigeratorIngredients, 3, ingredient)) {
		printf("%sBaker %d is looking to leave the refrigerator\n%s", color, bakerId, resetColor);
		recoverResource(REFRIGERATOR);
		printf("%sBaker %d left the refrigerator\n%s", color, bakerId, resetColor);
	}

	recoverIngredient(ingredient);

}

/**
 * Initializes the ingredients for a given recipe.
 *
 * @param recipe The recipe identifier. Valid values are:
 *               - COOKIE
 *               - PANCAKE
 *               - PIZZA
 *               - PRETZEL
 *               - CINROLL
 * @param initRecipe An array to store the initialized ingredients. The array should have enough space to hold all ingredient values.
 * @return A pointer to the initialized ingredients array.
 *
 * The function prints messages indicating the initialization process. If the recipe identifier is invalid (less than 0 or greater than 4),
 * the function prints an error message and exits the program.
 *
 * The ingredients are initialized as follows:
 * - COOKIE: FLOUR, SUGAR, MILK, BUTTER
 * - PANCAKE: FLOUR, SUGAR, BAKING_SODA, SALT, EGGS, MILK, BUTTER
 * - PIZZA: SUGAR, YEAST, SALT
 * - PRETZEL: FLOUR, SUGAR, YEAST, BAKING_SODA, SALT, EGGS
 * - CINROLL: FLOUR, SUGAR, SALT, CINNAMON, EGGS, BUTTER
 */
int* initRecipes(int recipe, int initRecipe[]) {

	printf("Initializing %d recipe\n", recipe);
	if (recipe < 0 || recipe > 4) {
		perror("Not a valid recipe");
		exit(1);
	}
	else {
		if (recipe == COOKIE) {
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


/**
 * @brief Checks if a given recipe is valid and contains at least one ingredient.
 *
 * This function checks if the provided recipe array has the correct size and
 * contains at least one ingredient. The recipe is considered valid if its size
 * is exactly 9. If the size is not 9, the function prints an error message and
 * terminates the program. If the size is valid, the function checks each element
 * of the recipe array to see if it contains the ingredient represented by the
 * value 1. If at least one ingredient is found, the function returns 1. Otherwise,
 * it returns 0.
 *
 * @param recipe An array of integers representing the recipe.
 * @param size The size of the recipe array.
 * @return int Returns 1 if the recipe contains at least one ingredient, otherwise 0.
 */
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

/**
 * @brief Toggles the presence of an ingredient in a recipe.
 *
 * This function checks if the specified ingredient is present in the recipe.
 * If the ingredient is present (indicated by a value of 1), it removes the ingredient
 * by setting its value to 0.
 *
 * @param recipe Pointer to an array representing the recipe, where each index corresponds to an ingredient.
 * @param ingredient The index of the ingredient to be toggled in the recipe array.
 */
void addIngredient(int* recipe, int ingredient) {

	if (recipe[ingredient] == 1) {
		recipe[ingredient] = 0;
	}
}

/**
 * @brief Retrieves an ingredient for a baker and updates the recipe.
 *
 * This function handles the process of a baker obtaining an ingredient,
 * updating the recipe with the ingredient, and then returning the ingredient.
 * It uses semaphores to manage access to the ingredients.
 *
 * @param bakerId The ID of the baker retrieving the ingredient.
 * @param recipe A pointer to the recipe array to be updated.
 * @param ingredient The ingredient to be retrieved and added to the recipe.
 * @param color The color code for printing messages.
 * @param resetColor The color code to reset the terminal color.
 * @return Always returns 1.
 */
int getIngredient(int bakerId, int* recipe, int ingredient, char* color, char* resetColor) {
	decSemaphores(bakerId, ingredient, color, resetColor);
	addIngredient(recipe, ingredient);
	printf("%sBaker %d got ingredient %d\n%s", color, bakerId, ingredient, resetColor);

	sleep(1);

	printf("%sBaker %d is returning ingredient %d\n%s", color, bakerId, ingredient, resetColor);

	incIngredientSemaphores(bakerId, ingredient, color, resetColor);

	return 1;
}

/**
 * @brief Checks if a specific ingredient is available in the recipe and attempts to get it.
 *
 * This function verifies if the provided recipe is valid and if the specified ingredient
 * is within the acceptable range. If the recipe is valid and the ingredient is available,
 * it attempts to get the ingredient.
 *
 * @param bakerId The ID of the baker attempting to get the ingredient.
 * @param recipe An array representing the recipe with ingredients.
 * @param size The size of the recipe array, which must be 9.
 * @param ingredient The index of the ingredient to check and get.
 * @return Returns 0 if the ingredient is not available or if the recipe is invalid.
 *         Otherwise, it returns the result of the getIngredient function.
 */
 //Return: Success of getting an ingredient
int checkIngredient(int bakerId, int* recipe, int size, int ingredient, char* color, char* resetColor) {

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

		return getIngredient(bakerId, recipe, ingredient, color, resetColor);

	}

	return 0;
}

/**
 * @brief Retrieves available ingredients for a given baker based on a recipe.
 *
 * This function iterates through the ingredients in the recipe and checks
 * their availability for the specified baker. It updates the status if any
 * ingredient is successfully obtained.
 *
 * @param bakerId The ID of the baker requesting the ingredients.
 * @param recipe A pointer to an array representing the recipe ingredients.
 * @return An integer indicating if any ingredient was successfully obtained
 *         (non-zero if successful, zero otherwise).
 */
 // Return: If an ingredient was successfully gotten
int getAvailableIngredients(int bakerId, int* recipe, char* color, char* resetColor) {
	int updated = 0;
	for (int i = 0; i < 9; i++) {
		updated |= checkIngredient(bakerId, recipe, 9, i, color, resetColor);
	}

	return updated;
}

/**
 * Checks if there is at least one remaining recipe in the list.
 *
 * This function iterates through the given array of recipes and checks if there
 * is at least one non-zero element, indicating that a recipe is remaining.
 *
 * @param recipes An array of integers representing the recipes.
 * @param length The length of the recipes array.
 * @return 1 if there is at least one remaining recipe, 0 otherwise.
 */
int isARecipeRemaining(int recipes[], int length) {
	for (int i = 0; i < length; i++) {
		if (recipes[i]) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Acquires the necessary mixing resources for a baker.
 *
 * This function attempts to acquire a mixer, a bowl, and a spoon for the baker
 * identified by the given bakerId. It logs the process of acquiring each resource.
 *
 * @param bakerId The ID of the baker attempting to acquire the resources.
 * @param tools An array where the acquired resources will be stored.
 * @param size The size of the tools array. Must be 3.
 * @param color The color code for the log messages.
 * @param resetColor The color code to reset the log messages.
 * @return 0 on success, or an error code on failure.
 */
int getMixingResources(int bakerId, int* tools, int size, char* color, char* resetColor) {
	if (size != 3) {
		perror("Not a valid set of tools");
	}

	printf("%sBaker %d is looking to acquire a mixer\n%s", color, bakerId, resetColor);
	tools[MIXER] = useResource(MIXER);
	printf("%sBaker %d acquired a mixer\n%s", color, bakerId, resetColor);

	printf("%sBaker %d is looking to acquire a bowl\n%s", color, bakerId, resetColor);
	tools[MIXER] = useResource(BOWL);
	printf("%sBaker %d acquired a bowl\n%s", color, bakerId, resetColor);

	printf("%sBaker %d is looking to acquire as spoon\n%s", color, bakerId, resetColor);
	tools[MIXER] = useResource(SPOON);
	printf("%sBaker %d acquired a spoon\n%s", color, bakerId, resetColor);

	return 0;
}

/**
 * @brief Returns the mixing resources used by a baker.
 *
 * This function recovers the resources used for mixing, which include:
 * - Mixer
 * - Bowl
 * - Spoon
 *
 * @param bakerId The ID of the baker returning the resources.
 * @return Always returns 0.
 */
int returnMixingResources(int bakerId) {
	recoverResource(MIXER);
	recoverResource(BOWL);
	recoverResource(SPOON);

	return 0;
}

/**
 * @brief Mixes ingredients using the specified tools.
 *
 * This function simulates a baker mixing ingredients together. It first acquires
 * the necessary mixing resources, then prints messages indicating the mixing process,
 * and finally releases the mixing resources.
 *
 * @param bakerId The ID of the baker performing the mixing.
 * @param tools An array of integers representing the tools required for mixing.
 * @param size The number of tools in the tools array.
 * @param color A string representing the color code for the output text.
 * @param resetColor A string representing the reset color code for the output text.
 * @return Always returns 0.
 */
int mixIngredients(int bakerId, int* tools, int size, char* color, char* resetColor) {
	getMixingResources(bakerId, tools, size, color, resetColor);

	printf("%sBaker %d is mixing the ingredients together\n%s", color, bakerId, resetColor);

	sleep(1);

	printf("%sBaker %d mixed all of the ingredients together\n%s", color, bakerId, resetColor);

	returnMixingResources(bakerId);

	return 0;
}


/**
 * @brief Cooks a recipe using the oven.
 *
 * This function simulates a baker using the oven to cook a specified recipe.
 * It prints messages to indicate the stages of the cooking process, including
 * looking to use the oven, using the oven, and finishing using the oven.
 * The function also simulates the time taken to cook the recipe by sleeping
 * for 3 seconds.
 *
 * @param bakerId The ID of the baker who is cooking the recipe.
 * @param recipe The ID of the recipe to be cooked.
 * @param color The color code to be used for the printed messages.
 * @param resetColor The color code to reset the printed messages to default.
 * @return Always returns 0.
 */
int cookRecipe(int bakerId, int recipe, char* color, char* resetColor) {
	printf("%sBaker %d is looking to use the oven to cook recipe %d%s\n", color, bakerId, recipe, resetColor);

	useResource(OVEN);

	printf("%sBaker %d is using the oven to cook recipe %d%s\n", color, bakerId, recipe, resetColor);

	sleep(3);

	printf("%sBaker %d finished using the oven to cook recipe %d%s\n", color, bakerId, recipe, resetColor);

	recoverResource(OVEN);

	return 0;
}

/**
 * @brief Simulates the actions of a baker in a multi-threaded environment.
 *
 * This function represents the logic for a baker who is responsible for preparing
 * various recipes. Each baker works on a set of predefined recipes and uses a set
 * of tools to complete them. The function runs in a loop until all recipes are completed.
 *
 * @param val A void pointer to an integer representing the baker's ID.
 * @return A void pointer, always returns NULL.
 *
 * The function performs the following steps:
 * 1. Initializes the recipes and tools.
 * 2. Iterates through each recipe, checking if it is completed.
 * 3. If a recipe is not completed, the baker attempts to gather the necessary ingredients.
 * 4. If the ingredients are available, the baker mixes and cooks the recipe.
 * 5. The process repeats until all recipes are completed.
 * 6. The function prints the status of the baker's progress and frees the memory allocated for the baker's ID.
 *
 * Note: When this function terminates, it will reclaim memory from the thread.
 */
void* simulateBaker(void* val) {
	//Put all baker logic in here
	//NOTE: When this function terminates, it will reclaim memory from the thread
	int* bakerIdRef = (int*)val;
	int bakerId = *bakerIdRef;


	// Select color based on bakerId
	const char* color = colors[bakerId % 7];
	const char* resetColor = "\033[0m";


	//Setup recipes
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

	key_t key = ftok(programPath, ramsiedSharedMemoryID);
	struct sharedMem sharedMemory;
	int failure = initSharedMemory(&sharedMemory, key, 2 * sizeof(int), 1);
	if (failure) {
		perror("Unable to get ramsied information");
	}

	int ramsiedBakerId = sharedMemory.address[0];
	int ramsiedRecipeId = sharedMemory.address[1];

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
			printf("%sBaker %d is working on making Cookies%s\n", color, bakerId, resetColor);
		}

		if (i == PANCAKE) {
			currentRecipe = pancake;
			printf("%sBaker %d is working on making Pancakes%s\n", color, bakerId, resetColor);
		}

		if (i == PIZZA) {
			currentRecipe = pizzaDough;
			printf("%sBaker %d is working on making Pizza Dough%s\n", color, bakerId, resetColor);
		}

		if (i == PRETZEL) {
			currentRecipe = softPretzel;
			printf("%sBaker %d is working on making Soft Pretzels%s\n", color, bakerId, resetColor);
		}

		if (i == CINROLL) {
			currentRecipe = cinnamonRoll;
			printf("%sBaker %d is working on making Cinnamon Rolls%s\n", color, bakerId, resetColor);
		}

		if (currentRecipe == NULL) {
			printf("Invalid index for recipe found within simulateBaker\n");
		}

		int isRecipeComplete = getAvailableIngredients(bakerId, currentRecipe, color, resetColor);
		recipesRemaining[i] = !isRecipeComplete;

		if (isRecipeComplete) {
			//Check if the baker is getting ramsied here. If they are, we only need to reset the array
			if (bakerId == ramsiedBakerId && i == ramsiedRecipeId) {
				printf("%sBaker %d has been ramsied on recipe %d%s\n", color, bakerId, i, resetColor);
				initRecipes(i, currentRecipe);
			} else {
				mixIngredients(bakerId, tools, 3, color, resetColor);

				cookRecipe(bakerId, i, color, resetColor);

				printf("%sBaker %d finished recipe %d%s\n", color, bakerId, i, resetColor);
			}
		}

		i++;
		i = i % 5;

	}

	printf("%sBaker %d has finished%s\n", color, bakerId, resetColor);

	free(bakerIdRef);
	return NULL;
}

/**
 * @brief Spawns a new thread to simulate a baker.
 *
 * This function allocates memory for the baker's ID, initializes it, and creates a new thread
 * to run the `simulateBaker` function. If the thread creation fails, an error message is printed
 * and the program exits.
 *
 * @param thread A pointer to a pthread_t variable where the thread ID will be stored.
 * @param bakerId An integer representing the ID of the baker.
 */
void spawnThread(pthread_t* thread, int bakerId) {

	int* id = malloc(sizeof(int));
	*id = bakerId;

	printf("Initializing baker %d\n", *id);

	int threadStatus = pthread_create(thread, NULL, simulateBaker, id);

	if (threadStatus != 0) {
		fprintf(stderr, "Thread create error %d: %s\n", threadStatus, strerror(threadStatus));

		exit(1);
	}

	//	pthread_detach(*thread);
}

/**
 * @brief Spawns a specified number of threads.
 *
 * This function initializes and spawns the given number of threads, each representing a baker.
 * It prints the number of bakers being initialized and then creates each thread by calling the
 * spawnThread function.
 *
 * @param threads A pointer to an array of pthread_t where the thread identifiers will be stored.
 * @param n The number of threads (bakers) to be spawned.
 */
void spawnThreads(pthread_t* threads, int n) {
	printf("Initializing %d bakers\n", n);
	//for(int bakerId = 1; bakerId <= n; bakerId++) {
	for (int bakerId = 0; bakerId < n; bakerId++) {
		spawnThread(&threads[bakerId], bakerId);
	}

}

/**
 * @brief Waits for all threads in the given array to complete.
 *
 * This function iterates over an array of pthread_t threads and calls
 * pthread_join on each one, ensuring that the main thread waits for
 * each thread to complete before continuing.
 *
 * @param threads An array of pthread_t representing the threads to wait for.
 * @param size The number of threads in the array.
 */
void waitForThreads(pthread_t* threads, int size) {
	for (int i = 0; i < size; i++) {
		pthread_join(threads[i], NULL);
	}
}

/**
 * @file program.c
 * @brief This program simulates a baking process with multiple bakers using semaphores for resource management and shared memory for communication.
 *
 * The program initializes several semaphores to manage access to kitchen resources such as mixers, pantry, refrigerator, bowls, spoons, and ovens.
 * It also initializes semaphores for various ingredients like flour, sugar, yeast, baking soda, salt, cinnamon, eggs, milk, and butter.
 *
 * The program uses shared memory to store the number of bakers and a randomly selected recipe.
 * It then creates a specified number of threads, each representing a baker, and waits for all threads to complete their tasks.
 *
 * @note The program handles the SIGINT signal to ensure proper cleanup of resources.
 *
 * @authors Steven Streasick, Zaydrian Price
 * @date 2024
 *
 * @param None
 * @return int: Returns 0 on successful execution.
 */
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
	key_t key = ftok(programPath, ramsiedSharedMemoryID);

	initSharedMemory(&sharedMemory, key, 2 * sizeof(int), 0);


	while (1) {
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

		sharedMemory.address[0] = rand() % bakers;
		sharedMemory.address[1] = rand() % 5;

		pthread_t threads[bakers];
		spawnThreads(threads, bakers);

		waitForThreads(threads, bakers);
		printf("All bakers have finished\n");
	}

	return 0;

}
