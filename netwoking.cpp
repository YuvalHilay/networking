#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <tchar.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#elif
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#define BUFLEN 512
#define PORT "27015" // the port client will be connecting to 
#define TIME_TILL_EXIT 15000
#define TIME_TILL_NEXT_REQUEST 1000
void* get_in_addr(struct sockaddr* sa);
void PrintMessage(char* error_str, int ReturnedValue, int ShowLastError);
void close_socket(int ConnectSocket);
void socket_cleanup();
int safe_gets(char* line, size_t size);
// add function
int send_messageToServer(int ConnectSocket, struct addrinfo* ServerAddress, char ip[BUFLEN], char sendbuf[BUFLEN], char* recvbuf);
int main(int argc, char* argv[])
{
#ifdef WIN32
	WSADATA wsaData;
#endif
	int ConnectSocket = INVALID_SOCKET;
	struct addrinfo hints;
	struct addrinfo* ServerAddress = NULL;
	int iResult;
	char sendbuf[BUFLEN];
	char recvbuf[BUFLEN];
	char finalanswer[BUFLEN];
	// Validate the parameters
	if (argc == 1) {
		fprintf(stderr, "client software can get server name or IP as argument.\n");
		fprintf(stderr, "default is loopback address: 127.0.0.1\n\n");
	}
	else if (argc != 2) {
		fprintf(stderr, "client software usage: %s server-name-or-IP\n or: %s    for loopback address 127.0.0.1\n", argv[0]);
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(TIME_TILL_EXIT);
		return 1;
	}
	// Initialize Winsock
#ifdef WIN32
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		PrintMessage("client: WSAStartup failed with error", iResult, 0);
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(TIME_TILL_EXIT);
		return 1;
	}
#endif

	// Resolve the server address and port
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;  //IPv4 address family
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	if (argc == 2)
		iResult = getaddrinfo(argv[1], PORT, &hints, &ServerAddress);
	else
		iResult = getaddrinfo("127.0.0.1", PORT, &hints, &ServerAddress);
	if (iResult != 0) {
		fprintf(stderr, "client: getaddrinfo failed with error: %s\n", gai_strerror(iResult));
		socket_cleanup();
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(TIME_TILL_EXIT);
		return 1;
	}

	// Create a SOCKET for connecting to server

	//if ai_family == AF_UNSPEC then socket sould be open for every connection attempt
	ConnectSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		PrintMessage("client:		socket failed", ConnectSocket, 1);
		socket_cleanup();
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(TIME_TILL_EXIT);
		return 1;
	}
	while (1) {
		// Send to
		iResult = send_messageToServer(ConnectSocket, ServerAddress, argv[1], "SENSOR GET 1", recvbuf);
		if (iResult)
			return 1;
		char sensorValue1[BUFLEN];
		// split the answer from the server by space
		char* token1 = strtok(recvbuf, " ");

		// loop through the string to extract all other tokens
		while (token1 != NULL) {

			// copy the value from token to sensorValue
			strcpy(sensorValue1, token1);

			// continue split with the rest of the recvbuf
			token1 = strtok(NULL, " ");
		}
		strcpy(finalanswer, "Water: ");
		strcat(finalanswer, sensorValue1);
		strcat(finalanswer, "[deg]");
		iResult = send_messageToServer(ConnectSocket, ServerAddress, argv[1], "SENSOR GET 2", recvbuf);
		if (iResult)
			return 1;
		char sensorValue2[BUFLEN];
		// split the answer from the server by space
		char* token2 = strtok(recvbuf, " ");

		// loop through the string to extract all other tokens
		while (token2 != NULL) {

			// copy the value from token to sensorValue
			strcpy(sensorValue2, token2);

			// continue split with the rest of the recvbuf
			token2 = strtok(NULL, " ");
		}
		strcat(finalanswer, ", Reactor: ");
		strcat(finalanswer, sensorValue2);
		strcat(finalanswer, "[deg]");
		iResult = send_messageToServer(ConnectSocket, ServerAddress, argv[1], "SENSOR GET 3", recvbuf);
		if (iResult)
			return 1;
		char sensorValue3[BUFLEN];
		// split the answer from the server by space
		char* token3 = strtok(recvbuf, " ");

		// loop through the string to extract all other tokens
		while (token3 != NULL) {

			// copy the value from token to sensorValue
			strcpy(sensorValue3, token3);

			// continue split with the rest of the recvbuf
			token3 = strtok(NULL, " ");
		}
		strcat(finalanswer, ", Pump current: ");
		strcat(finalanswer, sensorValue3);
		strcat(finalanswer, "[A]");
		printf("%s\n", finalanswer);
		Sleep(TIME_TILL_NEXT_REQUEST);
	}
	// cleanup

	freeaddrinfo(ServerAddress);
	close_socket(ConnectSocket);
	socket_cleanup();
	fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(TIME_TILL_EXIT);
	return 0;
}
int send_messageToServer(int ConnectSocket, struct addrinfo* ServerAddress, char ip[BUFLEN], char sendbuf[BUFLEN], char* recvbuf)
{
	int iResult;
	iResult = sendto(ConnectSocket, sendbuf, strlen(sendbuf), 0, ServerAddress->ai_addr, ServerAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		PrintMessage("client: send failed with error", iResult, 1);
		freeaddrinfo(ServerAddress);
		close_socket(ConnectSocket);
		socket_cleanup();
		fprintf(stderr, "\nprogram quit in 15 seconds\n"); Sleep(TIME_TILL_EXIT);
		exit(1);
	}
	// Receivefrom
	
	iResult = recvfrom(ConnectSocket, recvbuf, BUFLEN, 0, NULL, NULL);
	if (iResult < 0)
	{
		PrintMessage("client:		recv failed", iResult, 1);
		freeaddrinfo(ServerAddress);
		close_socket(ConnectSocket);
		socket_cleanup();
		exit(1);
	}
	recvbuf[iResult] = NULL;
	return 0;
}
// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void PrintMessage(char* error_str, int ReturnedValue, int ShowLastError)
{
#ifdef WIN32
	if (ShowLastError)
		fprintf(stderr, "%s. returned value: %d, last error: %ld.\n", error_str, ReturnedValue, WSAGetLastError());
	else
		fprintf(stderr, "%s: %d.\n", error_str, ReturnedValue);
#elif
	fprintf(stderr, "%s: %d.\n", error_str, ReturnedValue);
#endif
}
void close_socket(int ConnectSocket)
{
#ifdef WIN32
	closesocket(ConnectSocket);
#elif
	close(ConnectSocket);
#endif
}

void socket_cleanup()
{
#ifdef WIN32
	WSACleanup();
#endif
}

int safe_gets(char* line, size_t size)
{
	size_t i;
	for (i = 0; i < size - 1; ++i)
	{
		int ch = fgetc(stdin);
		if (ch == '\n' || ch == EOF) {
			break;
		}
		line[i] = ch;
	}
	line[i] = '\0';
	return i;
}
