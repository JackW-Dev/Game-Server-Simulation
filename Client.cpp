#define _CRT_SECURE_NO_WARNINGS

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <conio.h>
#include <time.h>

#pragma comment (lib, "Ws2_32.lib")

#define defaultBufferLen 512

using namespace std;

ifstream readFile;
ofstream writeFile;

struct client {
	int ID;
	SOCKET socket;
	char message[defaultBufferLen];
};

int clientOptions();

int main() {
	WSAData wsaData;
	addrinfo hints, * server = NULL, * result = NULL;
	SOCKET connectSock = INVALID_SOCKET;
	client thisClient = { -1, INVALID_SOCKET, "" };
	int returnCode, loopCounter = 0, timeout = 5000; //in milliseconds this is 5 seconds;
	boolean runClient = true;	
	string defaultIP, defaultPort;
	time_t t;

	/*Get server IP and Port*/
	readFile.open("serverDetails.txt");
	getline(readFile, defaultIP);
	getline(readFile, defaultPort);
	readFile.close();

	switch (clientOptions()) {
	case 1:
		/*Initialise Winsock*/
		cout << "Initialising Winsock." << endl;

		returnCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (returnCode != 0) {
			cout << "Failed to initialise Winsock. Error code: " << WSAGetLastError() << " Closing application." << endl;
			return 1;
		}
		cout << "Winsock successfully initialised." << endl;

		/*Set up hints*/
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		cout << "Connecting to server." << endl;

		/*Resolve server address and port information*/
		returnCode = getaddrinfo(defaultIP.c_str(), defaultPort.c_str(), &hints, &result);
		if (returnCode != 0) {
			cout << "Failed to get address info. Error code: " << returnCode << " Closing application." << endl;
			WSACleanup();
			return 1;
		}
		cout << "Address info successfully retrieved." << endl;

		/*Create socket to connect to server*/
		for (server = result; server != NULL; server = server->ai_next) {
			thisClient.socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
			if (thisClient.socket == INVALID_SOCKET) {
				cout << "Failed to create socket. Error code: " << WSAGetLastError() << "Closing application." << endl;
				WSACleanup();
				return 1;
			}
			returnCode = connect(thisClient.socket, server->ai_addr, (int)server->ai_addrlen);
			if (returnCode == SOCKET_ERROR) {
				cout << "Failed to connect uing socket. Error code: " << WSAGetLastError() << " Closing application." << endl;
				closesocket(thisClient.socket);
				thisClient.socket = INVALID_SOCKET;
				continue;
			}
			break;
		}

		freeaddrinfo(result);

		/*Locate server*/
		if (thisClient.socket == INVALID_SOCKET) {
			cout << "Unable to locate server. Closing application." << endl;
			WSACleanup();
			return 1;
		}
		cout << "Successfully located server." << endl;		

		setsockopt(thisClient.socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));
		
		/*Connect to server*/
		returnCode = recv(thisClient.socket, thisClient.message, defaultBufferLen, 0);
		if (returnCode == SOCKET_ERROR) {
			cout << "Failed to connect to server. Error code: " << WSAGetLastError() << " Closing application." << endl;	
			shutdown(thisClient.socket, 2);
			closesocket(thisClient.socket);
			WSACleanup();
			return 1;
		}		
		cout << "Successfully connected to server." << endl;
		
		/*Run client operation*/		
		thisClient.ID = atoi(thisClient.message);

		writeFile.open("client_" + to_string(thisClient.ID) + "_log.txt");

		cout << "Client ID: " << thisClient.ID << endl;

		t = time(NULL);
		writeFile << "Successfully connected to server. - " << asctime(localtime(&t)) << endl;
		writeFile << "Client ID: " << thisClient.ID << " - "<< asctime(localtime(&t)) << endl;

		while (runClient) {
			cout << "Press 'e' key to exit." << endl;
			if (_kbhit()) {
				if (_getch() == 'e' || _getch() == 'E') {	
					t = time(NULL);				
					cout << "Disconnecting from server." << endl;						
					writeFile << "Disconnecting from server. - " << asctime(localtime(&t)) << endl;
					runClient = false;
				}
				else {
					fflush(stdin);
				}
			}

			if (thisClient.socket != 0) {
				/*Socket is set*/
				returnCode = recv(thisClient.socket, thisClient.message, defaultBufferLen, 0);
				if (returnCode != SOCKET_ERROR) {					
					string message = "PING";
					send(thisClient.socket, message.c_str(), sizeof(message.c_str()), 0);

					t = time(NULL);
					cout << thisClient.message << endl;	
					writeFile << thisClient.message << " - " << asctime(localtime(&t)) << endl;

					if (_kbhit()) {
						if (_getch() == 'e' || _getch() == 'E') {
							t = time(NULL);
							cout << "Disconnecting from server." << endl;
							writeFile << "Disconnecting from server. - " << asctime(localtime(&t)) << endl;
							runClient = false;
						}
						else {
							cin.clear();
							cin.ignore(1000, '\n');
						}
					}
				}
				else {
					t = time(NULL);
					cout << "Recieve failed. Error code: " << WSAGetLastError() << " Closing application." << endl;
					writeFile << "Recieve failed. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
					closesocket(thisClient.socket);
					WSACleanup();
					return 1;
				}
			}
			else {
				t = time(NULL);
				cout << "Recieve failed. Error code: " << WSAGetLastError() << " Closing application." << endl;
				writeFile << "Recieve failed. Error code: " << WSAGetLastError() << " Closing application. - " << asctime(localtime(&t)) << endl;
				closesocket(thisClient.socket);
				WSACleanup();
				return 1;
			}
			if (WSAGetLastError() == WSAECONNRESET) {
				t = time(NULL);
				cout << "Server has disconnected. Closing application." << endl;
				writeFile << "Server has disconnected. Closing application. - " << asctime(localtime(&t)) << endl;
				closesocket(thisClient.socket);
				WSACleanup();
				return 1;
			}
		}
		
		/*Cleanup*/
		t = time(NULL);
		cout << "Closing socket." << endl;
		writeFile << "Closing socket. - " << asctime(localtime(&t)) << endl;

		returnCode = shutdown(thisClient.socket, SD_SEND);
		if (returnCode == SOCKET_ERROR) {
			t = time(NULL);
			cout << "Failed to close socket." << endl;
			writeFile << "Failed to close socket. - " << asctime(localtime(&t)) << endl;
			WSACleanup();
			return 1;
		}
		t = time(NULL);
		cout << "Closing client." << endl;
		writeFile << "Closing client. - " << asctime(localtime(&t)) << endl;
		writeFile.close();
		closesocket(thisClient.socket);
		WSACleanup();

		cin.clear();
		cin.ignore(1000, '\n');
		cout << "Enter any key to close the window." << endl;		
		cin.get();
		return 0;

	case 2:
		cout << "Closing client." << endl;
		return 0;
	}
}

/*
Funtion name - clientOptions()
Parameters - int joinLeave (flag for if the client is connected to server or not)
Returns - int
Functionality -
	Allow the client user to select whether they want to join, leave or continue on the server or if they wish to close the application
	Retuns an integer code for the validated result
Created - Jack Walker
Date - 29/03/2020
*/
int clientOptions() {
	int validationLoopCount, choice;

	cout << "Client" << endl;

	/*Input validation - Prior to connecting to server*/
	validationLoopCount = 0;
	do {
		if (validationLoopCount > 0) {
			cin.clear();
			cin.ignore(1000, '\n');
		}
		choice = 0;

		cout << "Please select an option from the following list." << endl;
		cout << "1 - " << "Connect to server" << endl;
		cout << "2 - " << "Exit application" << endl;
		cout << "Numerical ID of choice: ";
		cin >> choice;
		validationLoopCount++;
	} while (choice < 1 || choice > 2);
	return choice;
}
