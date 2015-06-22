// petebot.cpp : Defines the entry point for the console application.
//
#include "BWAPI.h"
#include "stdafx.h"
#include "k.h"

//using namespace BWAPI;

void onFrame();
int frameCount;

int _tmain(int argc, _TCHAR* argv[])
{
	int c = khpu("localhost", 1234, "myusername:mypassword"); // Connect to a Kdb+ server on the localhost port 1234 .
	k(-c, "a:2+2", (K)0);             // Asynchronously set a to be 4 on the server.
	K r = k(c, "b:til 1000000", (K)0);  // Synchronously set b to be a list up to 1000000.
	r = k(c, (S)0); // read incoming asynch
	return 0;
}
void connect2kdb()
{
	int c = khpun("localhost", 1234, "myname:mypassword", 1000); // timeout in mS
}
void onFrame()
{
	frameCount++;
	//Write information about the game state to the database
	//lets start by writing position data about the workers to the database
}

