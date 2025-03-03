/*

This module handles the traps, it has the following static functions: 
trapinit(), trapsyshandler(), trapmmhandler() and trapproghandler(). void trapinit(): 

void trapinit();

This function loads several entries in the EVT and it sets the new areas for the traps. 

void static trapsyshandler():

This function handles 9 different traps. It has a switch statement and each case calls a function. 
Two of the functions, waitforpclock and waitforio, are in int.c. The other seven are in syscall.c. 
Note that trapsyshandler passes a pointer to the old trap area to the functions in syscall.c and int.c 

void static trapmmhandler(): 
trapproghandler(): 

These functions pass up the traps or terminate the process.

*/

void trapinit();
void static trapsyshandler();
void static trapmmhandler();
void static trapproghandler(); 
