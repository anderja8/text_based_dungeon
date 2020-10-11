/*Program Name: anderja8.adventure.c
 *Course: CS344
 *Date: 1/27/2020
 *Description: Creates a directory of room files. The room files are
 *             used by the anderja8.adventure program to play a text
 *             based adventure game. The directory will be called
 *             anderja8.rooms.<process id> and will contain 7 files.
 *             Each of the 7 files will have room details, which means
 *             the name of the room, randomly chosen out of 10 names,
 *             a list of connected rooms, between 3 and 6 connections,
 *             and the type of room, start, mid, or end. There can be
 *             only 1 start room and only 1 end room.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

//Static ints that are used to set up the game
static int NUM_ROOMS = 7;

//Mutex lock that each of the threads will use to lock out the others
pthread_mutex_t lock;

//This struct will hold the core information for each room
//It is very similar to the struct buildrooms program uses.
//But, it has a string of room names as it's connection.
struct room {
	char name[9];
	char type[12];
	char connString[60];
	int numConn;
};

//This will implement a very simple linked list to keep track of the path taken throughout the game
struct pathNode {
	char name[9];
	struct pathNode* next;
};

//This function will initialize the passed room pointer to values specified by the passed file descriptor
void initRoom(struct room* currRoom) {
	memset(currRoom->name, '\0', sizeof(char)*9);
	memset(currRoom->type, '\0', sizeof(char)*12);
	currRoom->numConn = 0;
}

//Copies the most recently modified directory name containing the string: "anderja8.rooms." to the char pointer parameter.
//Heavily inspired by geeksforgeeks article on reading directories in C: 
//geeksforgeeks.org/c-program-list-files-sub-drectories-directory
void returnDir(char* dirName) {
	//Set up the variables
	DIR* dir = opendir(".");
	struct dirent* de;
	struct dirent* lastUpdatedDir;
	time_t lastUpdate = 0;
	struct stat dirStats;

	//Loop through all files/directories in current directory
	while ((de = readdir(dir)) != NULL) {
		//Consider only files having the substring "anderja8.rooms"
		if (strstr(de->d_name, "anderja8.rooms.") != NULL) {
			//Perform the stat call and check if the update time is newer than what is on record
			stat(de->d_name, &dirStats);
			if (lastUpdate < dirStats.st_mtime) {
				lastUpdate = dirStats.st_mtime;
				lastUpdatedDir = de;
			}
		}
	}
	
	//Assigning the directory name and returning
	strcpy(dirName, lastUpdatedDir->d_name);
	closedir(dir);
	return;
}

//Reads the 7 room files stored in the directory pointed to by parameter dir. Store the read information in
//an array of room pointers named roomArr.
void readDir(char* dirName, struct room** roomArr) {
	char pathStart[21];
	memset(pathStart, '\0', 21);
	strcpy(pathStart, dirName);
	DIR* dirList = opendir(dirName);
	struct dirent* de;	
	FILE* fp;
	char currLine[24];
	int numRooms = 0;

	while ((de = readdir(dirList)) != NULL) {
		if (strstr(de->d_name, "_room") != NULL && numRooms <= NUM_ROOMS) {
			//Generating the file path string
			char filePath[34];
			memset(filePath, '\0', 34);
			strcpy(filePath, pathStart);
			strcat(filePath, "/");
			strcat(filePath, de->d_name);

			//Open the file
			fp = fopen(filePath, "r");
			if (fp == NULL) {
				printf("Error opening file: %s\n", filePath);
				return;
			}

			//Read lines until the file pointer is NULL
			while (fgets(currLine, 30, fp) != NULL) {
				//Enter if statement to handle the different data declarations (name, connection, and type)
				if (strstr(currLine, "ROOM NAME:") != NULL) {
					//Setting the room name equal to it's value in the current line string
					strcpy(roomArr[numRooms]->name, &currLine[11]);
					//Removing the terminating new line character and replacing with null
					roomArr[numRooms]->name[strlen(roomArr[numRooms]->name)-1] = 0;
				}
				else if (strstr(currLine, "CONNECTION") != NULL) {
					if (roomArr[numRooms]->numConn == 0) {
						strcpy(roomArr[numRooms]->connString, &currLine[14]);
						roomArr[numRooms]->connString[strlen(roomArr[numRooms]->connString)-1] = 0;						
					}
					else {
						strcat(roomArr[numRooms]->connString, ", ");
						strcat(roomArr[numRooms]->connString, &currLine[14]);
						roomArr[numRooms]->connString[strlen(roomArr[numRooms]->connString)-1] = 0;
					}

					roomArr[numRooms]->numConn += 1;
				}
				else {
					strcpy(roomArr[numRooms]->type, &currLine[11]);						
					roomArr[numRooms]->type[strlen(roomArr[numRooms]->type)-1] = 0;
				}
	
				//Reset the line buffer
				memset(currLine, '\0', 30);
			}
			numRooms+=1;
			fclose(fp);
		}

	}

	closedir(dirList);
}

//prints room data to the console for the room pointed at by the parameter.
void printRoomData(struct room* currRoom) {
	printf("CURRENT LOCATION: %s\n", currRoom->name);
	printf("POSSIBLE CONNECTIONS: %s.\n", currRoom->connString);
	printf("WHERE TO? >");
}

//Validates the user's next destination selection. Returns 1 if accurate, 0
//if inaccurate.
int validateInput(struct room* currRoom, struct room** roomArr, char* userInput) {
	//Checking for extra characters in the input buffer, if any are found, returning 0
	//This method is based of a geeksforgeeks article titled, Clearing the Input Buffer in C
	int numExtraChars = 0;
	while ((getchar()) != '\n') {
		numExtraChars+=1;
	}

	if (numExtraChars > 0) {
		return 0;
	}

	if (strstr(currRoom->connString, userInput) != NULL) {
		int i;
		for (i=0; i < NUM_ROOMS; i++) {
			if (strcmp(userInput, roomArr[i]->name) == 0) {
				return 1;
			}
		}
	}
	
	return 0;
}

//Function to generate the currentTime.txt file.
//The second thread will point to this function.
//Accepts a mutex pointer, which the function will attempt to lock.
void* createTimeFile() {
	char timeString[200];
	time_t outputTime;

	//Attempting to lock the mutex
	pthread_mutex_lock(&lock);
	
	//Creating/Opening and truncating the currentTime.txt file
	int time_file = open("currentTime.txt", O_WRONLY | O_CREAT | O_TRUNC, 0640);
	if (time_file == -1) {
		printf("Error: currentTime.txt could not be opened/created.");
		perror("In createTimeFile()");
	}

	//Setting the outputTime variable to now
	outputTime = time(NULL);
	struct tm* timeStruct = localtime(&outputTime);

	//Converting the outputTime variable to a correctly formatted string, appending a newline character, and writing
	strftime(timeString, sizeof(timeString), "%l:%M%P, %A, %B %d, %Y" , timeStruct);
	strcat(timeString, "\n");
	write(time_file, timeString, strlen(timeString)*sizeof(char));
	pthread_mutex_unlock(&lock);
	return 0;
}

int main() {
	struct room* roomArr[NUM_ROOMS];
	int i;
	
	//Initialiaze the rooms
	for (i=0; i<NUM_ROOMS; i++) {
		roomArr[i] =(struct room*) malloc(sizeof(struct room));
		initRoom(roomArr[i]);
		if (roomArr[i] == NULL) {
			perror("malloc error.\n");
			return -1;
		}
	}
	
	//Determine the most recent directory and read the data out of it
	char dir[21];
	memset(dir, '\0', 21);
	returnDir(dir);
	//printf("directory to search is: %s\n", dir);
	readDir(dir, roomArr);

	/*
	//Print the room details, useful for debugging
	for (i=0; i<NUM_ROOMS; i++) {
		printf("name: %s\n", roomArr[i]->name);
		printf("type: %s\n", roomArr[i]->type);
		printf("connString: %s\n\n", roomArr[i]->connString);
	}
	*/

	//Determine the starting room, initialize the number of steps and path
	struct room* currRoom = 0;
	for (i=0; i<NUM_ROOMS; i++) {
		if (strcmp(roomArr[i]->type, "START_ROOM") == 0) {
			currRoom = roomArr[i];
			break;
		}
	}
	if (currRoom == 0) {
		perror("Error: starting room could not be found.\n");
	}
	int numSteps = 0;
	int gameWon = 0;
	struct pathNode* pathSentinel = malloc(sizeof(struct pathNode));
	struct pathNode* currNode = pathSentinel;

	//Begin the game
	while (gameWon == 0) {
		//Setting up variables and printing the next line
		char userInput[9];
		memset(userInput, '\0', 9);
		printRoomData(currRoom);

		//Getting and validating user input
		int inputValid = 0;
		while (inputValid == 0) {
			scanf("%s", userInput);

			printf("\n");
		
			//If the input is not time, validate it and continue)
			if (strcmp(userInput, "time") != 0) {
				inputValid = validateInput(currRoom, roomArr, userInput);

				if (inputValid == 0) {
					printf("HUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN\n\n");
					printRoomData(currRoom);
				}
			}
			//Else, work with the second thread to get the time	
			else {
				//Locking the mutex in this thread
				pthread_mutex_lock(&lock);

				//Creating a second thread to run the createTimeFile() function
				int timeThreadResult;
				pthread_t timeThread;
				timeThreadResult = pthread_create(&timeThread, NULL, createTimeFile, NULL);	

				//Unlocking the mutex and waiting for timeThread to finish
				pthread_mutex_unlock(&lock);
				
				pthread_join(timeThread, NULL);

				//Opening the currentTime.txt file for readin
				FILE* fp = fopen("currentTime.txt", "r");
				if (fp == NULL) {
					printf("Error reading currentTime.txt");
					perror("In main()");
				}

				//Printing the contents of its first line and then closing
				char timeLine[200];
				memset(timeLine, '\0', 200);
				fgets(timeLine, 200, fp);
				fclose(fp);

				//Printing the next room prompt
				printf("%s\n", timeLine);
				printf("WHERE TO? >");
			}
		}

		//Once valid input is recieved, move currRoom to this room, increment step counter and path
		for (i=0; i < NUM_ROOMS; i++) {
			if(strcmp(roomArr[i]->name, userInput) == 0) {
				currRoom = roomArr[i];
				break;
				printf("room found.\n");
			}
		}
		//If this is the first step, set the sentinel to this room
		if (numSteps == 0) {
			strcpy(pathSentinel->name, currRoom->name);
			pathSentinel->next = 0;
		}
		//Otherwise, allocate a new node, move the current node here, and set up it's member variables
		else {
			currNode->next = malloc(sizeof(struct pathNode));
			currNode = currNode->next;
			strcpy(currNode->name, currRoom->name);
			currNode->next = 0;
		}
		numSteps += 1;

		if (strcmp(currRoom->type, "END_ROOM") == 0) {
			gameWon = 1;
		}
	}

	//Print a congratulations and path information
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %u STEPS. YOUR PATH TO VICTORY WAS:\n", numSteps);
	
	currNode = pathSentinel;
	do {
		printf("%s\n", currNode->name);
		currNode = currNode->next;
	} while (currNode != 0);

	//Free dynamically allocated memory
	for (i=0; i<NUM_ROOMS; i++) {
		free(roomArr[i]);
	}

	currNode = pathSentinel;
	do {
		struct pathNode* prevNode = currNode;
		currNode = currNode->next;
		free(prevNode);
	} while (currNode != 0);

	pthread_mutex_destroy(&lock);

	return 0;	
}

