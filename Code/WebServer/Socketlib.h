#ifndef WEBSERVER_FILE_SOCKETLIB
#define WEBSERVER_FILE_SOCKETLIB
#include "Serialization.h"

#ifdef _WIN32
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0A00 //win10
	#endif

	#define WIN32_LEAN_AND_MEAN
	#include <Ws2tcpip.h>
	#include <winsock2.h>

	#pragma comment (lib, "Ws2_32.lib")

	typedef SOCKET webserver_socket;
	#define WEBSERVER_INVALID_SOCKET INVALID_SOCKET
	#define WEBSERVER_SOCKET_ERROR SOCKET_ERROR
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netinet/ip.h>
	#include <netinet/in.h>
	#include <netdb.h> // addrinfo
	#include <unistd.h> // close()

	typedef int webserver_socket;
	#define WEBSERVER_INVALID_SOCKET -1
	#define WEBSERVER_SOCKET_ERROR -1
#endif

void webserver_socketlib_init(const char* arport);
void webserver_socketlib_terminate();

[[noreturn]] void webserver_socketlib_error(const char* arfunction);

//SIZE/LENGTH PARAMETER IN ALL FUNCTIONS EXPECTS AMOUNT OF BYTES

//makes a new socket
webserver_socket webserver_socketlib_open();
//waits for incoming connection, returns a socket for reading
webserver_socket webserver_socketlib_wait_for_connection(webserver_socket* asock);

//returns false if time to close and writes the amount of recieved bytes to apdatabuflength
bool webserver_socketlib_listen(webserver_socket asock, void* apdatabuf, size_t* apdatabuflength, const size_t amaxdatabuflength);
//returns amount of bytes sent
size_t webserver_socketlib_send(webserver_socket asock, void* apdata, const size_t apdatalength);

//only shuts the connection down
void webserver_socketlib_disconnect(webserver_socket asock);
//only closes the socket
void webserver_socketlib_deactivate(webserver_socket asock);
//closes and shuts the socket down
void webserver_socketlib_close(webserver_socket asock);

#endif

