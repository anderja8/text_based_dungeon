# Background
This is a simple, text-based dungeon game that I wrote for my OS course. It consists of two executable files,
one to build the dungeon rooms, and one to play the game with the dungeon rooms. The purpose is to move through
the rooms until you find the end game room. Your path taken will then be ouput to the console.

# Installation
`git clone`  
`make game` to compile the two programs  
`./anderja8.buildrooms` to generate a directory of the game rooms  
`ls` to get the name of the new directory  
`./anderja8.adventure <directory name>` to start up the game.  

# Game Rules
Simply type the name of the room you wish to enter next into the prompt. If you would like to view the time, you can enter `time`
and the current time will be printed to the screen (this demos simple multithreading in c). Continue until you reach the end room
or get bored.

Each time buildrooms is run, the connections between the rooms is randomized, so each time will be a new adventure.

When you are done, enter `make clean` to remove all generated files.
