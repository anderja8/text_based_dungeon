/*Program Name: anderja8.buildrooms.cs
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

//Static ints that are used to set up the game
static int NUM_ROOMS = 7;
static int NUM_NAMES = 10;

//This struct will hold the core information for each room
struct room {
	char name[9];
	char type[11];
	int numConn;
	struct room* conn[6];
};

//This function will initialize the passed room pointer to default values
void initRoom(struct room* currRoom) {
	memset(currRoom->name, '\0', 9);
	memset(currRoom->type, '\0', 11);
	strcpy(currRoom->type,"MID_ROOM");
	currRoom->numConn = 0;
	int i;
	for (i=0; i<6; i++) {
		currRoom->conn[i]=0;
	}
}

//This function will randomly return 1 of the remaining names in the name array
//The taken array is a paired boolean array used to determine which names are still available
//Namesleft is an int representing how many names are remaining in the name array.
char* assignName(char** nameArr, int* takenArr, int namesLeft) {
	//Get a random number, the unused name that is read at the randNumth iteration will be returned
	int randNum = rand() % namesLeft;
	int i = 0;
	
	//Loop over the name array
	for (i = 0; i < NUM_NAMES; i++) {
		//If the name is not taken and randNum = 0, take the name and set the taken array at that index to true
		if (randNum == 0 && takenArr[i] == 0) {
			takenArr[i] = 1;
			return nameArr[i];
		}
		//Else, if the name is not taken, decrement randNum
		else if (takenArr[i] == 0) {
			randNum-=1;
		}
	}

	//If we made it through the for loop, something is wrong. Print an error to the user.
	printf("Error in assignName function. Function should return before exiting for loop.");
	return "ERROR";
}

//Function to create a connection between 2 rooms
//Does nothing if the connection cannot be made, either
//because idx1=idx2 or room pointed to by idx1 or
//idx2 already has 6 connections. Else, increments
//the numConn struct variable, adds a pointer to
//the connected row to the conn array, and returns.
void generateConnection(struct room** roomArr, int idx1, int idx2) {
	//Verify that connection can be created
	if (idx1 == idx2) {
		return;
	}
	
	else if (roomArr[idx1]->numConn == 6 || roomArr[idx2]->numConn == 6) {
		return;
	}

	int i;
	for (i=0; i < roomArr[idx1]->numConn; i++) {
		if (roomArr[idx1]->conn[i] == roomArr[idx2]) {
			return;
		}
	}
	
	//If we passed the check, add the pointer and increment numConn for both room indices
	for (i=0; i < 6; i++) {
		if (roomArr[idx1]->conn[i]==0) {
			roomArr[idx1]->conn[i] = roomArr[idx2];
			roomArr[idx1]->numConn += 1;
			break;
		}
	}
	
	for (i=0; i < 6; i++) {
		if (roomArr[idx2]->conn[i]==0) {
			roomArr[idx2]->conn[i] = roomArr[idx1];
			roomArr[idx2]->numConn += 1;
			break;
		}
	}
	
	return;
}

//The main function, this calls the functions to generate the room data and then writes it into a directory
int main() {
	//Initializing the variables
	char* roomNames[10] = {"Dungeon","Cavern","Throne","Arboreum","Forest","Tunnel","Field","Pit","Nursery","Grotto"};
	int nameTaken[10] = {0};
	int i, j;
	int startRoom = rand() % NUM_ROOMS;
	int endRoom = rand() % NUM_ROOMS;
	while (startRoom == endRoom) {
		endRoom = rand() % NUM_ROOMS;
	}
	struct room* gameRooms[NUM_ROOMS];

	//Seeding my random generator
	srand(time(0));

	//Looping through the array of rooms and initialzing them
	for (i=0; i < NUM_ROOMS; i++) {
		//Allocate the memory
		gameRooms[i] = malloc(sizeof(struct room));
		//Call init function
		initRoom(gameRooms[i]);
		//Generate the room name
		strcpy(gameRooms[i]->name,assignName(roomNames, nameTaken, NUM_NAMES - i));
		//If the room is a start or end room, change the type
		if (i == startRoom) {
			memset(gameRooms[i]->type, '\0', 11);
			strcpy(gameRooms[i]->type,"START_ROOM");
		}
		else if (i == endRoom) {
			memset(gameRooms[i]->type, '\0', 11);
			strcpy(gameRooms[i]->type,"END_ROOM");
		}
	}

	for (i=0; i < NUM_ROOMS; i++) {
		//Generate the connections
		while (gameRooms[i]->numConn < 3) {
			int idx2 = rand() % NUM_ROOMS;
			generateConnection(gameRooms, i, idx2);
		}
	}
	
	/*Debugging loop to print details to the console
	//Print the final room details
	for (i=0; i < NUM_ROOMS; i++) {
		printf("room %u is of type %s, has name %s, and %u connections\n", i, gameRooms[i]->type, gameRooms[i]->name, gameRooms[i]->numConn);
  		int j;
		for (j=0; j < gameRooms[i]->numConn; j++) {
			printf("Connection %u: %s\n", j, gameRooms[i]->conn[j]->name);
		}
	}
	*/	

	//Get the PID and create a directory
	pid_t currPid = getpid();
	char pidString[6]; //The pid_max value in proc/sys/kernel is 49152 so a 6 char buffer is good
	sprintf(pidString, "%d", currPid);
	char dirName[22] = "anderja8.rooms.";
	strcat(dirName, pidString);
	mkdir(dirName, 0740);

	//Create a file for each room, placing the file in the directory
	int file_descriptor;
	char roomPath[35];
	for (i=0; i < NUM_ROOMS; i++) {
		//Getting the path for the file names
		memset(roomPath, '\0', 37);
		strcpy(roomPath, dirName);
		strcat(roomPath, "/");
		strcat(roomPath, gameRooms[i]->name);
		strcat(roomPath, "_room");
	
		//Opening the file and printing an error if something goes wrong
		file_descriptor = open(roomPath, O_WRONLY | O_CREAT | O_TRUNC, 0640);
		if (file_descriptor == -1) {
			printf("Error: file %s could not be created.\n", roomPath);
			perror("In main()");
		}

		//Debugging line
		//printf("Room path is: %s and length is: %u\n", roomPath, strlen(roomPath));

		//Writing the data from the gamerooms array into the file
		//Starting with the name
		char nameLine[21];
		memset(nameLine, '\0', 21);
		strcpy(nameLine, "ROOM NAME: ");
		strcat(nameLine, gameRooms[i]->name);
		strcat(nameLine, "\n");
		write(file_descriptor, nameLine, strlen(nameLine)*sizeof(char));
		
		//Looping through each connection
		for (j=0; j < gameRooms[i]->numConn; j++) {
			char connectionLine[24];
			memset(connectionLine, '\0', 24);
			strcpy(connectionLine, "CONNECTION ");
			char connNumber[2];
			memset(connNumber, '\0', 2);
			sprintf(connNumber, "%d", j+1);
			strcat(connectionLine, connNumber);
			strcat(connectionLine, ": ");
			strcat(connectionLine, gameRooms[i]->conn[j]->name);
			strcat(connectionLine, "\n");
			write(file_descriptor, connectionLine, strlen(connectionLine)*sizeof(char));
		}

		//Writing the room type
		char typeLine[30];
		memset(typeLine, '\0', 30);
		strcpy(typeLine, "ROOM TYPE: ");
		strcat(typeLine, gameRooms[i]->type);
		strcat(typeLine, "\n\0");
		write(file_descriptor, typeLine, strlen(typeLine)*sizeof(char));
	}

	//Free the dynamic memory
	for (i=0; i < NUM_ROOMS; i++) {
		free(gameRooms[i]);
	}

	return 0;
}
