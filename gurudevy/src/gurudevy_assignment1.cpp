/**
 * @gurudevy_assignment1
 * @author  Gurudev Yalagala <gurudevy@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * the License, or (at your option) any later version.
 * published by the Free Software Foundation; either version 2 of
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <strings.h>
#include <sstream>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <stdarg.h>

#include "../include/global.h"
#include "../include/logger.h"

using namespace std;

namespace INPUT_ARGS
{
	enum COMMANDS
	{
		AUTHOR,
		IP,
		PORT,
		LIST,
		STATISTICS,
		BLOCKED
	};
} // namespace INPUT_ARGS

struct Message
{
	string messageOpType;
	string messageDestinationIP;
	string messageSourceIP;
	string messageInfo;
};

struct Client
{
	int clientNumber;
	string clientIPAddr;
	int clientPort;
	int socketFD;
	string clientHostName;
	bool logStatus = false;
	int sentMessageCount = 0;
	int rcevMessageCount = 0;
	vector<string> ipBlocks;
	vector<int> ipBlocksPorts;
	int currentBlocks = 0;
	vector<Message> messageQueue;
	int queuedMessagesCount = 0;
} * CURRENT_CLIENT_LIST[5];

struct ServerMetaInfo
{
	struct Message serverMessage;
	struct Client clientList[5];
	int currentClientCount = 0;
	int currentClientIndex;
} sendMessage, recvMessage;

int CURRENT_CLIENT_COUNT = 0;

void serverSideRequest(int portNumner);
void clientSideRequest(int portNumner);
int createSocketConnection(int portNumber, sockaddr_in &newServerAddress);
void createClientList();
void displayIpAddress();
void displayPortNumber(int portNumber);
void displayClientList();
void displayClientStats();

INPUT_ARGS::COMMANDS getCommand(string const &inputCommand)
{
	if (inputCommand == "AUTHOR")
		return INPUT_ARGS::AUTHOR;
	if (inputCommand == "IP")
		return INPUT_ARGS::IP;
	if (inputCommand == "PORT")
		return INPUT_ARGS::PORT;
	if (inputCommand == "LIST")
		return INPUT_ARGS::LIST;
	if (inputCommand == "STATISTICS")
		return INPUT_ARGS::STATISTICS;
	if (inputCommand == "BLOCKED")
		return INPUT_ARGS::BLOCKED;
}

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/* Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	// Check the args count
	if (argc == 3)
	{
		int portNumber = atoi(argv[2]);

		switch (*argv[1])
		{
		case 's':
			serverSideRequest(portNumber);
			break;

		case 'c':
			/* code */
			clientSideRequest(portNumber);
			break;

		default:
			exit(-1);
		}
	}
	else
	{
		exit(-1);
	}

	return 0;
}

void serverSideRequest(int portNumber)
{
	struct sockaddr_in newServerAddress;
	int serverSocket, bindSocket, listenToSocket;
	// Create a socket
	serverSocket = createSocketConnection(portNumber, newServerAddress);
	// Bind the socket.
	bind(serverSocket, (struct sockaddr *)&newServerAddress, sizeof(newServerAddress));
	if (bindSocket < 0)
	{
		perror("SOCKET BINDING FAILURE\n");
	}
	else
	{
		printf("SOCKET BINDING SUCCESSFUL\n");
		// Listen to socket
		listenToSocket = listen(serverSocket, CLIENT_BACKLOG);
		if (listenToSocket)
		{
			perror("SOCKET LISTENING FAILURE\n");
		}
		else
		{
			printf("SOCKET LISTENING SUCCESSFUL\n");
		}
	}

	// Inti and set the file descriptors to zeros.
	fd_set serverFDs, clientFDs;
	FD_ZERO(&serverFDs);
	FD_ZERO(&clientFDs);
	FD_SET(serverSocket, &serverFDs);
	FD_SET(IS_NULL, &serverFDs);
	printf("SETTING FDs\n");
	// Init clients list.
	// createClientList();
	while (!IS_NULL)
	{
		memcpy(&clientFDs, &serverFDs, sizeof(serverFDs));

		if (select(serverSocket + 1, &clientFDs, NULL, NULL, NULL) < 0)
		{
			perror("SELECT FD FAILURE");
		}
		else
		{
			printf("GETTING INPUT COMMANDS\n");
			printf("----------------------\n");
			for (int sourceFD = 0; sourceFD <= serverSocket; sourceFD++)
			{
				memset(&sendMessage, '\0', sizeof(sendMessage));

				if (FD_ISSET(sourceFD, &clientFDs))
				{
					if (sourceFD == IS_NULL)
					{
						char inputCommand[CMD_ARG_SIZE];
						if (fgets(inputCommand, sizeof(inputCommand), stdin) != NULL)
						{
							inputCommand[sizeof(inputCommand) - 1] = '\0';
							switch (getCommand(inputCommand))
							{
							case INPUT_ARGS::AUTHOR:
								cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
								printf("[AUTHOR:SUCCESS]\n");
								cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "gurudevy");
								printf("I, %s, have read and understood the course academic integrity policy.\n", "gurudevy");
								cse4589_print_and_log("[AUTHOR:END]\n");
								printf("[AUTHOR:END]\n");
								break;

							case INPUT_ARGS::IP:
								displayIpAddress();
								break;

							case INPUT_ARGS::PORT:
								displayPortNumber(portNumber);
								break;

							case INPUT_ARGS::LIST:
								displayClientList();
								break;

							case INPUT_ARGS::STATISTICS:
								displayClientStats();
								break;

							case INPUT_ARGS::BLOCKED:
								// printClientBlockList();
								break;

							default:
								printf("DEFAULT");
								break;
							}
						}
						else
						{
							exit(-1);
						}
					}
				}
			}
		}
	}
}

void clientSideRequest(int portNumber)
{
	struct sockaddr_in newServerAddress;
	// Create a socket
	int serverSocket = createSocketConnection(portNumber, newServerAddress);
	// Bind the socket.
	int bindSocket = bind(serverSocket, (struct sockaddr *)&newServerAddress, sizeof(newServerAddress));

	if (bindSocket < 0)
	{
		perror("SOCKET BINDING FAILURE");
	}
	else
	{
		printf("SOCKET BINDING SUCCESSFUL");
	}

	// Listen to socket
	int listenToSocket = listen(serverSocket, CLIENT_BACKLOG);
	if (listenToSocket)
	{
		perror("SOCKET LISTENING FAILURE");
	}
	else
	{
		printf("SOCKET LISTENING SUCCESSFUL");
	}

	// Inti and set the file descriptors to zeros.
	fd_set serverFDs, clientFDs;
	FD_ZERO(&serverFDs);
	FD_ZERO(&clientFDs);
	FD_SET(serverSocket, &serverFDs);
	FD_SET(IS_NULL, &serverFDs);

	// // Init clients list.
	// createClientList();
	while (!IS_NULL)
	{
		memcpy(&clientFDs, &serverFDs, sizeof(serverFDs));

		if (select(serverSocket + 1, &clientFDs, NULL, NULL, NULL) < 0)
		{
			perror("SELECT FD FAILURE");
		}
		else
		{

			for (int sourceFD = 0; sourceFD <= serverSocket; sourceFD++)
			{
				memset(&sendMessage, '\0', sizeof(sendMessage));

				if (FD_ISSET(sourceFD, &clientFDs))
				{
					if (sourceFD == IS_NULL)
					{
						char inputCommand[CMD_ARG_SIZE];
						if (fgets(inputCommand, sizeof(inputCommand), stdin) != NULL)
						{
							inputCommand[sizeof(inputCommand) - 1] = '\0';
							switch (getCommand(inputCommand))
							{
							case INPUT_ARGS::AUTHOR:
								/* code */
								break;

							case INPUT_ARGS::IP:
								/* code */
								break;

							case INPUT_ARGS::PORT:
								/* code */
								break;

							case INPUT_ARGS::LIST:
								/* code */
								break;

							case INPUT_ARGS::STATISTICS:
								/* code */
								break;

							case INPUT_ARGS::BLOCKED:
								/* code */
								break;

							default:
								break;
							}
						}
						else
						{
							exit(-1);
						}
					}
				}
			}
		}
	}
}

int createSocketConnection(int portNumber, sockaddr_in &newServerAddress)
{
	int socketConnect = socket(AF_INET, SOCK_STREAM, 0);
	if (socketConnect < 0)
	{
		perror("SOCKET CREATION FAILURE\n");
	}
	else
	{
		// Only if zero
		bzero(&newServerAddress, sizeof(newServerAddress));
		// process to fill the struct
		newServerAddress.sin_family = AF_INET;
		newServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
		newServerAddress.sin_port = htonl(portNumber);
	}
	return socketConnect;
}
void displayIpAddress()
{
	struct addrinfo tempHint, *responseAddr, *temp2;
	int status;
	char ipAddr[INET_ADDRSTRLEN];

	memset(&tempHint, 0, sizeof tempHint);
	tempHint.ai_family = AF_INET;
	tempHint.ai_socktype = SOCK_STREAM;
	status = getaddrinfo("www.google.com", NULL, &tempHint, &responseAddr);
	if (status != 0)
	{
		for (temp2 = responseAddr; temp2 != NULL; temp2 = temp2->ai_next)
		{
			void *addr;
			char *ipver;
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)temp2->ai_addr;
			// convert the IP to a string and print it:
			inet_ntop(temp2->ai_family, &(ipv4->sin_addr), ipAddr, sizeof(ipAddr));
			cse4589_print_and_log("[IP:SUCCESS]\n");
			cse4589_print_and_log("IP:%s\n", ipAddr);
			cse4589_print_and_log("[IP:END]\n");
		}
	}
	else
	{
		cse4589_print_and_log("[IP:ERROR]\n");
		cse4589_print_and_log("[IP:END]\n");
	}

	freeaddrinfo(responseAddr); // free the linked list
}

void displayPortNumber(int portNumber)
{
	cse4589_print_and_log("[PORT:SUCCESS]\n");
	cse4589_print_and_log("PORT:%d\n", portNumber);
	cse4589_print_and_log("[PORT:END]\n");
}

void displayClientList()
{
	cse4589_print_and_log("[PORT:SUCCESS]\n");
	for (int i = 0; i < CURRENT_CLIENT_COUNT; i++)
	{
		int nextClient = i + 1;
		cse4589_print_and_log("%d |HostName:%s | IP: %s |  PortNumber: %d \n", nextClient,
							  &CURRENT_CLIENT_LIST[i]->clientHostName,
							  &CURRENT_CLIENT_LIST[i]->clientIPAddr,
							  &CURRENT_CLIENT_LIST[i]->clientPort);
	}
	cse4589_print_and_log("[CLIENT:SUCCESS]\n");
}

void displayClientStats()
{
}

// void createClientList()
// {
// 	for (auto i = 0; i < 5; i++)
// 	{
// 		//
// 		struct Client **currentClient = &CURRENT_CLIENT_LIST[i];
// 		for (auto j = 0; j < 4; i++)
// 		{
// 			(*currentClient)->ipBlocks.push_back("TEMP_IP");
// 			(*currentClient)->ipBlocksPorts.push_back(66000); //! CHANGE NUMBER
// 		}
// 		(*currentClient)->currentBlocks = 0;
// 		(*currentClient)->currentBlocks = 66000;
// 		(*currentClient)->queuedMessagesCount = 0;
// 		(*currentClient)->sentMessageCount = 0;
// 		(*currentClient)->rcevMessageCount = 0;
// 		for (auto k = 0; k < MAX_BUFFER_MESSAGES; k++)
// 		{
// 			struct Message tempMessage = {
// 				messageOpType : "DEFAULT_MESSAGE",
// 				messageSourceIP : "TEMP_IP",
// 				messageDestinationIP : "TEMP_IP",
// 				messageInfo : "NULL"
// 			};
// 			(*currentClient)->messageQueue.push_back(tempMessage);
// 		}

// 		CURRENT_CLIENT_COUNT++;
// 	}
// 	cout << CURRENT_CLIENT_LIST[3] << endl;
// }
