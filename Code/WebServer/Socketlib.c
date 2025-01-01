#include "Socketlib.h"

const char* cport = NULL;

void webserver_socketlib_init(const char* arport) {
	cport = arport;
#ifdef _WIN32
	WSADATA winsockdata;
	if(!WSAStartup(MAKEWORD(1,1), &winsockdata)) {
		fprintf("Winsock startup failed.\n");
		exit(EXIT_FAILURE);
	}
#endif
}

void webserver_socketlib_terminate() {
#ifdef _WIN32
	WSACleanup();
#endif
}

[[noreturn]] void webserver_socketlib_error(const char* arfunction) {
#ifdef _WIN32
	fprintf(stdout, "Error when calling %s, %s\n", arfunction, WSAGetLastError());
#else
	fprintf(stdout, "Error when calling %s, %s\n", arfunction, strerror(errno));
#endif
	webserver_socketlib_terminate();
	exit(EXIT_FAILURE);
}

webserver_socket webserver_socketlib_open() {
	struct addrinfo hint;
	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	hint.ai_flags = AI_PASSIVE;

	struct addrinfo* result = NULL;
	webserver_socket error = getaddrinfo(NULL, cport, &hint, &result); //get information about addresss
	if(error) {
		webserver_socketlib_error("getaddrinfo");
	}

	webserver_socket sockethandle = WEBSERVER_INVALID_SOCKET;
	sockethandle = socket(result->ai_family, result->ai_socktype, result->ai_protocol); //get socket
	if(sockethandle == WEBSERVER_INVALID_SOCKET) {
		webserver_socketlib_error("socket");
	}

	int reuse = 1;
	error = setsockopt(sockethandle, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(error) {
		webserver_socketlib_error("setsockopt (reuse)");
	}

	error = bind(sockethandle, result->ai_addr, result->ai_addrlen); //bind socket to address
	if(error == WEBSERVER_SOCKET_ERROR) {
		webserver_socketlib_error("bind");
	}

	freeaddrinfo(result); //free data
	return sockethandle; //return our socket
}

webserver_socket webserver_socketlib_wait_for_connection(webserver_socket* asock) {
	//listen for connection
	webserver_socket error = listen(*asock, SOMAXCONN);
	if(error == WEBSERVER_SOCKET_ERROR) {
		webserver_socketlib_error("listen");
	}

	//we use "server" socket to find connection, then we change sockets to communicate
	webserver_socket new_socket = accept(*asock, NULL, NULL);
	if(new_socket == WEBSERVER_INVALID_SOCKET) {
		webserver_socketlib_error("accept");
	}

	return new_socket;
}
bool webserver_socketlib_listen(webserver_socket asock, void* apdatabuf, size_t* apdatabuflength, const size_t amaxdatabuflength) {
	int error = recv(asock, apdatabuf, amaxdatabuflength, 0);
	if(error < 0) {
		fprintf(stdout, "Socket recieve error!\n");
		webserver_socketlib_error("recv");
	}
	else if(error == 0) {
		//close connection!
		return false;
	}
	else {
		*apdatabuflength = error; //write size
		return true;
	}
}
size_t webserver_socketlib_send(webserver_socket asock, void* apdata, const size_t apdatalength) {
	int error = send(asock, apdata, apdatalength, 0);
	if(error == WEBSERVER_SOCKET_ERROR) {
		webserver_socketlib_error("send");
	}
	return error;
}

void webserver_socketlib_disconnect(webserver_socket asock) {
	webserver_socket status;
#ifdef _WIN32
	status = shutdown(asock, SD_BOTH);
#else
	status = shutdown(asock, SHUT_RDWR);
#endif
	if (status == WEBSERVER_SOCKET_ERROR) webserver_socketlib_error("shutdown");
}
void webserver_socketlib_deactivate(webserver_socket asock) {
	webserver_socket status;
#ifdef _WIN32
	status = closesocket(asock);
#else
	status = close(asock);
#endif
	if (status == WEBSERVER_SOCKET_ERROR) webserver_socketlib_error("close(socket)");
}
void webserver_socketlib_close(webserver_socket asock) {
	webserver_socket status;
#ifdef _WIN32
	status = shutdown(asock, SD_BOTH);
	if (status == 0) { status = closesocket(asock); }
	else webserver_socketlib_error("shutdown");
#else
	status = shutdown(asock, SHUT_RDWR);
	if (status == 0) { status = close(asock); }
	else webserver_socketlib_error("shutdown");
#endif
}
