
#include "stdafx.h"
#include <BWAPI.h>
#include <BWAPI/Client.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <winsock2.h>

#define KXVER 3
#include "k.h"

#include<stdlib.h>
#include<string.h>
#include<sys/types.h>


using namespace BWAPI;

void drawStats();
void drawBullets();
void drawVisibilityData();
void showPlayers();
void showForces();
void assignIdle();
void assignIfWorker(BWAPI::UnitInterface * u);
void mineNearest(BWAPI::UnitInterface * u, Unitset minerals);
bool show_bullets;
bool show_visibility_data;
void kdbStuff();

void reconnect()
{
	while (!BWAPIClient.connect())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{ 1000 });
	}
}


int main(int argc, const char* argv[])
{
	kdbStuff();
	std::cout << "Connecting..." << std::endl;;
	reconnect();
	while (true)
	{
		std::cout << "waiting to enter match" << std::endl;
		while (!Broodwar->isInGame())
		{
			BWAPI::BWAPIClient.update();
			if (!BWAPI::BWAPIClient.isConnected())
			{
				std::cout << "Reconnecting..." << std::endl;;
				reconnect();
			}
		}
		std::cout << "starting match!" << std::endl;
		Broodwar->sendText("Hello world!");
		Broodwar << "The map is " << Broodwar->mapName() << ", a " << Broodwar->getStartLocations().size() << " player map" << std::endl;
		// Enable some cheat flags
		Broodwar->enableFlag(Flag::UserInput);
		// Uncomment to enable complete map information
		//Broodwar->enableFlag(Flag::CompleteMapInformation);

		show_bullets = true;
		show_visibility_data = true;

		if (Broodwar->isReplay())
		{
			Broodwar << "The following players are in this replay:" << std::endl;;
			Playerset players = Broodwar->getPlayers();
			for (auto p : players)
			{
				if (!p->getUnits().empty() && !p->isNeutral())
					Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;
			}
		}
		else
		{
			if (Broodwar->enemy())
				Broodwar << "The match up is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

			//send each worker to the mineral field that is closest to it
			Unitset units = Broodwar->self()->getUnits();
			Unitset minerals = Broodwar->getMinerals();
			for (auto &u : units)
			{
				if (u->getType().isWorker())
				{
					Unit closestMineral = nullptr;

					for (auto &m : minerals)
					{
						if (!closestMineral || u->getDistance(m) < u->getDistance(closestMineral))
							closestMineral = m;
					}
					if (closestMineral)
						u->rightClick(closestMineral);
				}
				else if (u->getType().isResourceDepot())
				{
					//if this is a center, tell it to build the appropiate type of worker
					u->train(Broodwar->self()->getRace().getWorker());
				}
			}
		}
		while (Broodwar->isInGame())
		{
			for (auto &e : Broodwar->getEvents())
			{
				switch (e.getType())
				{
				case EventType::MatchEnd:
					if (e.isWinner())
						Broodwar << "I won the game" << std::endl;
					else
						Broodwar << "I lost the game" << std::endl;
					break;
				case EventType::SendText:
					if (e.getText() == "/show bullets")
					{
						show_bullets = !show_bullets;
					}
					else if (e.getText() == "/show players")
					{
						showPlayers();
					}
					else if (e.getText() == "/show forces")
					{
						showForces();
					}
					else if (e.getText() == "/show visibility")
					{
						show_visibility_data = !show_visibility_data;
					}
					else
					{
						Broodwar << "You typed \"" << e.getText() << "\"!" << std::endl;
					}
					break;
				case EventType::ReceiveText:
					Broodwar << e.getPlayer()->getName() << " said \"" << e.getText() << "\"" << std::endl;
					break;
				case EventType::PlayerLeft:
					Broodwar << e.getPlayer()->getName() << " left the game." << std::endl;
					break;
				case EventType::NukeDetect:
					if (e.getPosition() != Positions::Unknown)
					{
						Broodwar->drawCircleMap(e.getPosition(), 40, Colors::Red, true);
						Broodwar << "Nuclear Launch Detected at " << e.getPosition() << std::endl;
					}
					else
						Broodwar << "Nuclear Launch Detected" << std::endl;
					break;
				case EventType::UnitCreate:
					if (!Broodwar->isReplay())
						Broodwar << "A " << e.getUnit()->getType() << " [" << e.getUnit() << "] has been created at " << e.getUnit()->getPosition() << std::endl;
					else
					{
						// if we are in a replay, then we will print out the build order
						// (just of the buildings, not the units).
						if (e.getUnit()->getType().isBuilding() && e.getUnit()->getPlayer()->isNeutral() == false)
						{
							int seconds = Broodwar->getFrameCount() / 24;
							int minutes = seconds / 60;
							seconds %= 60;
							Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, e.getUnit()->getPlayer()->getName().c_str(), e.getUnit()->getType().c_str());
						}
					}
					break;
				case EventType::UnitDestroy:
					if (!Broodwar->isReplay())
						Broodwar->sendText("A %s [%p] has been destroyed at (%d,%d)", e.getUnit()->getType().c_str(), e.getUnit(), e.getUnit()->getPosition().x, e.getUnit()->getPosition().y);
					break;
				case EventType::UnitMorph:
					if (!Broodwar->isReplay())
						Broodwar->sendText("A %s [%p] has been morphed at (%d,%d)", e.getUnit()->getType().c_str(), e.getUnit(), e.getUnit()->getPosition().x, e.getUnit()->getPosition().y);
					else
					{
						// if we are in a replay, then we will print out the build order
						// (just of the buildings, not the units).
						if (e.getUnit()->getType().isBuilding() && e.getUnit()->getPlayer()->isNeutral() == false)
						{
							int seconds = Broodwar->getFrameCount() / 24;
							int minutes = seconds / 60;
							seconds %= 60;
							Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, e.getUnit()->getPlayer()->getName().c_str(), e.getUnit()->getType().c_str());
						}
					}
					break;
				case EventType::UnitShow:
					if (!Broodwar->isReplay())
						Broodwar->sendText("A %s [%p] has been spotted at (%d,%d)", e.getUnit()->getType().c_str(), e.getUnit(), e.getUnit()->getPosition().x, e.getUnit()->getPosition().y);
					break;
				case EventType::UnitHide:
					if (!Broodwar->isReplay())
						Broodwar->sendText("A %s [%p] was last seen at (%d,%d)", e.getUnit()->getType().c_str(), e.getUnit(), e.getUnit()->getPosition().x, e.getUnit()->getPosition().y);
					break;
				case EventType::UnitRenegade:
					if (!Broodwar->isReplay())
						Broodwar->sendText("A %s [%p] is now owned by %s", e.getUnit()->getType().c_str(), e.getUnit(), e.getUnit()->getPlayer()->getName().c_str());
					break;
				case EventType::SaveGame:
					Broodwar->sendText("The game was saved to \"%s\".", e.getText().c_str());
					break;
				}
			}

			if (show_bullets)
				drawBullets();

			if (show_visibility_data)
				drawVisibilityData();

			drawStats();
			Broodwar->drawTextScreen(300, 0, "FPS: %f", Broodwar->getAverageFPS());

			BWAPI::BWAPIClient.update();
			if (!BWAPI::BWAPIClient.isConnected())
			{
				std::cout << "Reconnecting..." << std::endl;
				reconnect();
			}
			assignIdle();
		}
		std::cout << "Game ended" << std::endl;
	}
	std::cout << "Press ENTER to continue..." << std::endl;
	std::cin.ignore();
	return 0;
}

void connect2db()
{

}
void assignIdle()
{
	Unitset units = Broodwar->self()->getUnits();
	Unitset minerals = Broodwar->getMinerals();
	for (auto &u : units)
	{
		if (u->isIdle())
		{
			if (u->getType().isWorker())
			{
				mineNearest(u, minerals);
			}
		}
	}
}
void mineNearest(BWAPI::UnitInterface * u, Unitset minerals)
{
	Unit closestMineral = nullptr;

	for (auto &m : minerals)
	{
		if (!closestMineral || u->getDistance(m) < u->getDistance(closestMineral))
			closestMineral = m;
	}
	if (closestMineral)
		u->rightClick(closestMineral);
}
void drawStats()
{
	int line = 0;
	Broodwar->drawTextScreen(5, 0, "I have %d units:", Broodwar->self()->allUnitCount());
	for (auto& unitType : UnitTypes::allUnitTypes())
	{
		int count = Broodwar->self()->allUnitCount(unitType);
		if (count)
		{
			Broodwar->drawTextScreen(5, 16 * line, "- %d %s%c", count, unitType.c_str(), count == 1 ? ' ' : 's');
			++line;
		}
	}
}

void drawBullets()
{
	for (auto &b : Broodwar->getBullets())
	{
		Position p = b->getPosition();
		double velocityX = b->getVelocityX();
		double velocityY = b->getVelocityY();
		Broodwar->drawLineMap(p, p + Position((int)velocityX, (int)velocityY), b->getPlayer() == Broodwar->self() ? Colors::Green : Colors::Red);
		Broodwar->drawTextMap(p, "%c%s", b->getPlayer() == Broodwar->self() ? Text::Green : Text::Red, b->getType().c_str());
	}
}

void drawVisibilityData()
{
	int wid = Broodwar->mapHeight(), hgt = Broodwar->mapWidth();
	for (int x = 0; x < wid; ++x)
		for (int y = 0; y < hgt; ++y)
		{
			if (Broodwar->isExplored(x, y))
				Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Broodwar->isVisible(x, y) ? Colors::Green : Colors::Blue);
			else
				Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Colors::Red);
		}
}

void showPlayers()
{
	Playerset players = Broodwar->getPlayers();
	for (auto p : players)
		Broodwar << "Player [" << p->getID() << "]: " << p->getName() << " is in force: " << p->getForce()->getName() << std::endl;
}

void showForces()
{
	Forceset forces = Broodwar->getForces();
	for (auto f : forces)
	{
		Playerset players = f->getPlayers();
		Broodwar << "Force " << f->getName() << " has the following players:" << std::endl;
		for (auto p : players)
			Broodwar << "  - Player [" << p->getID() << "]: " << p->getName() << std::endl;
	}
}
void kdbStuff()
{
	I c = khpu("localhost", 5000, "myusername:mypassword"); // Connect to a Kdb+ server on the localhost port 1234 .
	K row = knk(3, ks((S)"ibm"), ki(93), ki(300));
	//k(c, "'update", ks((S)"tab"), row, (K)0);

	
	k(-c, "insert", ks(ss("tab")), row, (K)0);
	k(c, "", (K)0); // flush buffers 

	//std::cout << "connecting to kdb" << std::endl;
	
	//std::cout << "connected!" << std::endl;
	//k(-c, "a:2+2", (K)0);             // Asynchronously set a to be 4 on the server.
	//K r = k(c, "b:til 100", (K)0);  // Synchronously set b to be a list up to 1000000.
	//std::cout << "b set" << std::endl;
	//r = k(c, (S)0); // read incoming asynch
	//std::cout << "c read" << std::endl;
	//printf("%d\n", r->i);

	//int retval;
	//K x;
	//K r;
	//
	//int fd = khp("localhost", 9999); // In a production system, check the return value
	//fd_set fds;
	//struct timeval tv;
	//while (1){
	//	tv.tv_sec = 5;
	//	tv.tv_usec = 0;
	//	FD_ZERO(&fds);
	//	FD_SET(fd, &fds);
	//	retval = select(fd + 1, &fds, NULL, NULL, &tv);
	//	if (retval == -1)
	//		perror("select()"), exit(1);
	//	else if (retval){
	//		printf("Data is available now.\n");
	//		if (FD_ISSET(fd, &fds)){
	//			x = k(fd, (S)0);
	//			printf("%d\n", x->i);
	//		}
	//	}
	//	else
	//		printf("No data within five seconds.\n");
	//}
	//kclose(fd);
}