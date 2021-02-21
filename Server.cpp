#define _CRT_SECURE_NO_WARNINGS

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <algorithm>

#pragma comment (lib, "Ws2_32.lib")

#define defaultIP "127.0.0.1"
#define defaultPort "3504"
#define defaultBufferLen 512

using namespace std;

/*Structure definitions*/
struct serverSettings {
	int gamemode = 0, map = 0, difficulty = 0, minPlayers = 0, maxPlayers = 0, maxConnections = 0;
};

struct client {
	int ID;
	SOCKET socket;
	boolean isRunning;
};

/*Function prototypes*/
void serverSetup(serverSettings*);
void clientProcess(client&, serverSettings&);

ofstream writeFile;

int connectedClients = 0, currentPlayers = 0, tempIndex = -1;
static string gamemodeNames[3] = { "Deathmatch", "Capture The Flag", "Blood Diamond" };
static string mapNames[3] = { "Tower", "Outback", "Hereford Base" };
static string difficultyNames[3] = { "Easy", "Challenging", "Veteran" };
time_t t;

int main() {
	serverSettings settings;

	serverSetup(&settings);

	writeFile.open("serverDetails.txt");
	writeFile << defaultIP << endl;
	writeFile << defaultPort << endl;
	writeFile.close();

	WSAData wsaData;
	addrinfo hints, * server = NULL;
	SOCKET listenerSock = INVALID_SOCKET, clientSock = INVALID_SOCKET;
	string message = "";
	vector<client> clients(settings.maxConnections);
	vector<thread> clientThreads(settings.maxConnections);
	int returnCode, tempClientID, clientID = 1, connectionsMade = 0;
	boolean runServer = true;

	writeFile.open("serverLog.txt");	

	cout << "Server setup: " << endl;
	cout << "IP: " << defaultIP << endl;
	cout << "PORT: " << defaultPort << endl;
	cout << "Gamemode: " << gamemodeNames[settings.gamemode - 1] << endl;
	cout << "Map: " << mapNames[settings.map - 1] << endl;
	cout << "Difficulty: " << difficultyNames[settings.difficulty - 1] << endl;
	cout << "Minimum players: " << settings.minPlayers << endl;
	cout << "Maximum players: " << settings.maxPlayers << endl;
	cout << "Maximum connections: " << settings.maxConnections << endl;

	writeFile << "Server setup: " << endl;
	writeFile << "IP: " << defaultIP << endl;
	writeFile << "PORT: " << defaultPort << endl;
	writeFile << "Gamemode: " << gamemodeNames[settings.gamemode - 1] << endl;
	writeFile << "Map: " << mapNames[settings.map - 1] << endl;
	writeFile << "Difficulty: " << difficultyNames[settings.difficulty - 1] << endl;
	writeFile << "Minimum players: " << settings.minPlayers << endl;
	writeFile << "Maximum players: " << settings.maxPlayers << endl;
	writeFile << "Maximum connections: " << settings.maxConnections << endl;

	/*Initialise Winsock*/	
	returnCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (returnCode != 0) {
		t = time(NULL);
		cout << "Failed to initialise Winsock. Error code: " << WSAGetLastError() << " Closing application." << endl;		
		writeFile << "Failed to initialise Winsock. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
		writeFile.close();
		return 1;
	}
	t = time(NULL);
	cout << "Winsock successfully initialised." << endl;
	writeFile << "Winsock successfully initialised. - " << asctime(localtime(&t)) << endl;

	/*Set up hints*/
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	/*Get address information*/
	returnCode = getaddrinfo(defaultIP, defaultPort, &hints, &server);
	if (returnCode != 0) {
		t = time(NULL);
		cout << "Failed to get address info. Error code: " << WSAGetLastError() << " Closing application." << endl;
		writeFile << "Failed to get address info. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
		writeFile.close();
		WSACleanup();
		return 1;
	}
	t = time(NULL);
	cout << "Address information recieved." << endl;
	writeFile << "Address information recieved. - " << asctime(localtime(&t)) << endl;

	/*Create listener socket*/
	listenerSock = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	if (listenerSock == INVALID_SOCKET) {
		t = time(NULL);
		cout << "Failed to create listener socker. Error code: " << WSAGetLastError() << " Closing application." << endl;
		writeFile << "Failed to create listener socker. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
		writeFile.close();
		freeaddrinfo(server);
		closesocket(listenerSock);
		WSACleanup();
		return 1;
	}
	t = time(NULL);
	cout << "Listener socket created." << endl;
	writeFile << "Listener socket created. - " << asctime(localtime(&t)) << endl;

	setsockopt(listenerSock, SOL_SOCKET, SO_REUSEADDR, 0, 0);

	/*Bind address to listener socker*/
	returnCode = bind(listenerSock, server->ai_addr, (int)server->ai_addrlen);
	if (returnCode == SOCKET_ERROR) {
		t = time(NULL);
		cout << "Failed to bind address to listener port. Error code: " << WSAGetLastError() << " Closing application." << endl;
		writeFile << "Failed to bind address to listener port. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
		writeFile.close();
		freeaddrinfo(server);
		closesocket(listenerSock);
		WSACleanup();
		return 1;
	}
	t = time(NULL);
	cout << "Address bound to listener socket." << endl;
	writeFile << "Address bound to listener socket. - " << asctime(localtime(&t)) << endl;	

	/*Listen for inbound connections*/
	t = time(NULL);
	cout << "Listening for connections." << endl;
	writeFile << "Listening for connections. - " << asctime(localtime(&t)) <<  endl;
	listen(listenerSock, SOMAXCONN);

	/*Initialise client list*/
	for (int i = 0; i < settings.maxConnections; i++) {
		clients[i] = { -1, INVALID_SOCKET, false};
	}
		
	/*Run clients*/
	while (runServer) {
		t = time(NULL);
		cout << "Connected Clients: " << connectedClients << endl;
		writeFile << "Connected Clients: " << connectedClients << " - " << asctime(localtime(&t)) << endl;
		cout << "Current players: " << currentPlayers << endl;
		writeFile << "Current players: " << currentPlayers << " - " << asctime(localtime(&t)) << endl;
		/*Add new connection*/
		if (connectedClients < settings.maxConnections) {	
			/*Check if there are any spectators that could be converted to players*/
			while (currentPlayers < settings.maxPlayers && connectedClients > currentPlayers) {
				for (int i = 0; i < settings.maxConnections; i++) {
					if (currentPlayers == settings.maxPlayers) continue;
					if (clients[i].isRunning == false && clients[i].ID != -1) {

						clients[i].isRunning = true;
						currentPlayers++;
					}
				}
			}

			/*Only accept new connections if there is space*/
			clientSock = accept(listenerSock, NULL, NULL);

			connectedClients = 0;
			tempClientID = -1;

			for (int i = 0; i < settings.maxConnections; i++) {
				if (clients[i].socket == INVALID_SOCKET && tempClientID == -1) {
					clients[i].socket = clientSock;
					clients[i].ID = clientID;
					clients[i].isRunning = false;
					tempClientID = i;
					clientID++;
				}
				if (clients[i].socket != INVALID_SOCKET) {
					connectedClients++;
					connectionsMade++;
				}

			
			}
			if (currentPlayers < settings.maxPlayers) {
				clients[tempClientID].isRunning = true;
				currentPlayers++;
			}

			t = time(NULL);
			cout << "Client " << clients[tempClientID].ID << " accepted." << endl;
			writeFile << "Client " << clients[tempClientID].ID << " accepted. - " << asctime(localtime(&t)) << endl;

			if (connectedClients < settings.minPlayers) {
				t = time(NULL);
				cout << "Waiting for more clients to start match. " << connectedClients << " / " << settings.minPlayers << " in queue." << endl;
				writeFile << "Waiting for more clients to start match. " << connectedClients << " / " << settings.minPlayers << " in queue. - " << asctime(localtime(&t)) << endl;
			}

			message = to_string(clients[tempClientID].ID);
			send(clients[tempClientID].socket, message.c_str(), strlen(message.c_str()), 0);

			clientThreads[tempClientID] = thread(clientProcess, ref(clients[tempClientID]), ref(settings));

			clientThreads[tempClientID].detach();
			clientThreads[tempClientID].~thread();
		}
		else {
			/*Cant accept any more clients - Close socket until there is space*/
			t = time(NULL);
			cout << "Closing listener socket whilst server is full." << endl;
			writeFile << "Closing listener socket whilst server is full. - " << asctime(localtime(&t)) << endl;
			closesocket(listenerSock);
			while (connectedClients == settings.maxConnections) {			
			}
			/*Set up hints*/
			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = AI_PASSIVE;

			/*Get address information*/
			returnCode = getaddrinfo(defaultIP, defaultPort, &hints, &server);
			if (returnCode != 0) {
				t = time(NULL);
				cout << "Failed to get address info. Error code: " << WSAGetLastError() << " Closing application." << endl;
				writeFile << "Failed to get address info. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
				writeFile.close();
				WSACleanup();
				return 1;
			}
			t = time(NULL);
			cout << "Address information recieved." << endl;
			writeFile << "Address information recieved. - " << asctime(localtime(&t)) << endl;

			/*Create listener socket*/
			listenerSock = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
			if (listenerSock == INVALID_SOCKET) {
				t = time(NULL);
				cout << "Failed to create listener socker. Error code: " << WSAGetLastError() << " Closing application." << endl;
				writeFile << "Failed to create listener socker. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
				writeFile.close();
				freeaddrinfo(server);
				closesocket(listenerSock);
				WSACleanup();
				return 1;
			}
			t = time(NULL);
			cout << "Listener socket created." << endl;
			writeFile << "Listener socket created. - " << asctime(localtime(&t)) << endl;

			setsockopt(listenerSock, SOL_SOCKET, SO_REUSEADDR, 0, 0);

			/*Bind address to listener socker*/
			returnCode = bind(listenerSock, server->ai_addr, (int)server->ai_addrlen);
			if (returnCode == SOCKET_ERROR) {
				t = time(NULL);
				cout << "Failed to bind address to listener port. Error code: " << WSAGetLastError() << " Closing application." << endl;
				writeFile << "Failed to bind address to listener port. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
				writeFile.close();
				freeaddrinfo(server);
				closesocket(listenerSock);
				WSACleanup();
				return 1;
			}
			t = time(NULL);
			cout << "Address bound to listener socket." << endl;
			writeFile << "Address bound to listener socket. - " << asctime(localtime(&t)) << endl;

			/*Listen for inbound connections*/
			t = time(NULL);
			cout << "Listening for connections." << endl;
			writeFile << "Listening for connections. - " << asctime(localtime(&t)) << endl;
			listen(listenerSock, SOMAXCONN);
		}
	}
	
	/*Clean up*/
	closesocket(listenerSock);
	for (int i = 0; i < settings.maxPlayers; i++) {
		clientThreads[i].detach();
		closesocket(clients[i].socket);
	}
	WSACleanup();
	t = time(NULL);
	cout << "Server closed successfully." << endl;
	writeFile << "Server closed successfully. - " << asctime(localtime(&t)) << endl;
	writeFile.close();

	return 0;
}

void clientProcess(client& thisClient, serverSettings& settings) {
	int returnCode, loopCount = 0, pingResponse[3] = { 0, 0, 0 };
	string message = "";
	char returnMessage[defaultBufferLen];
	boolean cont = true;
	time_t currentTime, previousTime;

	previousTime = time(NULL);

	while (cont) {
		memset(returnMessage, 0, defaultBufferLen);
		currentTime = time(NULL);
		message = "";

		if (currentTime - previousTime == 5) {
			/*If 5 seconds has passed send a message and loop for a reply of PING (this is a very basic ping system)*/
			if (thisClient.isRunning) {
				if (connectedClients < settings.minPlayers) {
					message = "Waiting for players. " + to_string(settings.minPlayers - connectedClients) + " more players needed to start.";
					returnCode = send(thisClient.socket, message.c_str(), strlen(message.c_str()), 0);
				}
				else {
					message = "Now playing " + gamemodeNames[settings.gamemode - 1] + " on " + mapNames[settings.map - 1]
						+ ". Difficulty: " + difficultyNames[settings.difficulty - 1];
					returnCode = send(thisClient.socket, message.c_str(), strlen(message.c_str()), 0);
				}
			}
			else {
				message = "Spectating " + gamemodeNames[settings.gamemode - 1] + " on " + mapNames[settings.map - 1]
					+ ". Difficulty: " + difficultyNames[settings.difficulty - 1];
				returnCode = send(thisClient.socket, message.c_str(), strlen(message.c_str()), 0);
			}

			returnCode = recv(thisClient.socket, returnMessage, defaultBufferLen, 0);
			if (returnCode != SOCKET_ERROR) {
				if (strcmp(returnMessage, "PING") == 0) {
					pingResponse[loopCount] = 1;
				}				
				else {
					pingResponse[loopCount] = 0;
				}
			}
			else {
				pingResponse[loopCount] = 0;
			}
			if (pingResponse[0] == 0 && pingResponse[1] == 0 && pingResponse[2] == 0) {
				/*No response for 15 seconds - Assume client is dead*/
				cont = false;
			}

			previousTime = currentTime;
			cout << "Message sent to client number " << thisClient.ID << endl;

			cout << "Client number " << thisClient.ID << " pingResponse[" << loopCount << "] = " << pingResponse[loopCount] << endl;

			loopCount++;
			if (loopCount == 3) {
				loopCount = 0;
			}
		}
	}
	/*Loop is only exited if client has disconnected*/
	if (thisClient.isRunning) {
		currentPlayers--;
	}
	t = time(NULL);
	connectedClients--;
	cout << "Client " << thisClient.ID << " has disconnected." << endl;
	writeFile << "Client " << thisClient.ID << " has disconnected. - " << asctime(localtime(&t)) << endl;
	shutdown(thisClient.socket, 2);
	closesocket(thisClient.socket);
	thisClient.socket = INVALID_SOCKET;	
	thisClient.ID = -1;
	
}

/*
Funtion name - serverSettings()
Parameters - serverSD* settings, string array gamemodeNames, string array mapNames, string array difficultyNames
Returns - serverSD structure (serverSetupDetails -- Stores game rule details in one place)
Functionality -
	Take a valid serverSD
	Prompt user for setup info
	Validate user input
	Add valid values to serverSD
Created - Jack Walker
Date - 25/03/2020
*/
void serverSetup(serverSettings* temp) {
	int validationLoopCount;

	cout << "Server Config" << endl;

	/*Gamemode input validation*/
	validationLoopCount = 0;
	do {
		if (validationLoopCount > 0) {
			cin.clear();
			cin.ignore(1000, '\n');
		}
		temp->gamemode = 0;

		cout << "Please select a gamemode from the following list." << endl;
		cout << "1 - " << gamemodeNames[0] << endl;
		cout << "2 - " << gamemodeNames[1] << endl;
		cout << "3 - " << gamemodeNames[2] << endl;
		cout << "Numerical ID of gamemode: ";
		cin >> temp->gamemode;
		validationLoopCount++;
	} while (temp->gamemode < 1 || temp->gamemode > 3);

	/*Map input validation*/
	validationLoopCount = 0;
	do {
		if (validationLoopCount > 0) {
			cin.clear();
			cin.ignore(1000, '\n');
		}
		temp->map = 0;

		cout << "Please select a map from the following list." << endl;
		cout << "1 - " << mapNames[0] << endl;
		cout << "2 - " << mapNames[1] << endl;
		cout << "3 - " << mapNames[2] << endl;
		cout << "Numerical ID of map: ";
		cin >> temp->map;
		validationLoopCount++;
	} while (temp->map < 1 || temp->map > 3);

	/*Difficulty input validation*/
	validationLoopCount = 0;
	do {
		if (validationLoopCount > 0) {
			cin.clear();
			cin.ignore(1000, '\n');
		}
		temp->difficulty = 0;

		cout << "Please select a difficulty from the following list." << endl;
		cout << "1 - " << difficultyNames[0] << endl;
		cout << "2 - " << difficultyNames[1] << endl;
		cout << "3 - " << difficultyNames[2] << endl;
		cout << "Numerical ID of difficulty: ";
		cin >> temp->difficulty;
		validationLoopCount++;
	} while (temp->difficulty < 1 || temp->difficulty > 3);

	/*Minimum players input validation*/
	/*NOTE - Minimum players must be 2 or more*/
	validationLoopCount = 0;
	do {
		if (validationLoopCount > 0) {
			cin.clear();
			cin.ignore(1000, '\n');
		}
		temp->minPlayers = 0;

		cout << "Minimum players (must be 2 or more): ";
		cin >> temp->minPlayers;
		validationLoopCount++;
	} while (temp->minPlayers < 2);

	/*Maximum players input validation*/
	/*NOTE - Maximum players must be larger than or equal to minimum players*/
	validationLoopCount = 0;
	do {
		if (validationLoopCount > 0) {
			cin.clear();
			cin.ignore(1000, '\n');
		}
		temp->maxPlayers = 0;

		cout << "Maximum players (must be " << temp->minPlayers << " or more): ";
		cin >> temp->maxPlayers;
		validationLoopCount++;
	} while (temp->maxPlayers < temp->minPlayers);

	/*Maximum connections input validation*/
	/*NOTE - Maximum connections must be larger than or equal to maximum players*/
	validationLoopCount = 0;
	do {
		if (validationLoopCount > 0) {
			cin.clear();
			cin.ignore(1000, '\n');
		}
		temp->maxConnections = 0;

		cout << "Maximum connections (must be " << temp->maxPlayers << " or more): ";
		cin >> temp->maxConnections;
		validationLoopCount++;
	} while (temp->maxConnections < temp->maxPlayers);
}