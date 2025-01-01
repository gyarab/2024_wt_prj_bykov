#include "System.h"

//It would be possible to set up an array of pointers and just add things here, but since we only need to free 2 structs i think this is fine

struct {
	webserver_socket sock;
	webserver_socket readsock;
	webserver_wid_database* wid;
	webserver_page_info* free1;

} webserver_socketlib_keyboard_interrupt_handler_values;

void webserver_set_interrupt_handler(webserver_socket asock, webserver_wid_database* apwid, webserver_page_info* apfreepi1) {
	webserver_socketlib_keyboard_interrupt_handler_values.sock = asock;
	webserver_socketlib_keyboard_interrupt_handler_values.wid = apwid;
	webserver_socketlib_keyboard_interrupt_handler_values.free1 = apfreepi1;
}

void webserver_set_interrupt_handler_readsock(webserver_socket areadsock) {
	webserver_socketlib_keyboard_interrupt_handler_values.readsock = areadsock;
}
void webserver_set_interrupt_handler_pi(webserver_page_info* apfreepi1) {
	webserver_socketlib_keyboard_interrupt_handler_values.free1 = apfreepi1;
}

[[noreturn]] void webserver_keyboard_interrupt_handler(int asignal) {
	fprintf(stdout, "Interrupt recieved, closing sockets and DB...");
	webserver_wid_close(webserver_socketlib_keyboard_interrupt_handler_values.wid);
	webserver_socketlib_close(webserver_socketlib_keyboard_interrupt_handler_values.sock);
	webserver_socketlib_close(webserver_socketlib_keyboard_interrupt_handler_values.readsock);
	webserver_free_page_info(webserver_socketlib_keyboard_interrupt_handler_values.free1);
	webserver_socketlib_terminate();
	fprintf(stdout, "done!\n");
	exit(EXIT_SUCCESS);
}

[[noreturn]] void webserver_print_help_menu() {
	fprintf(stdout,
			"NPWS WebServer\n"
			"(c) Martin Bykov 2024-2025\n"
			"Usage:\n"
			"(1) ./webserver <port>\n"
			"(2) ./webserver --help\n"
			"    ./webserver -h\n"
			"----------\n"
			"Usage 1:\n"
			"Start the web server on the local machine with the port <port>.\n"
			"----------\n"
			"Usage 2:\n"
			"Displays this help menu and exits successfully.\n"
			"----------\n"
	);
	exit(EXIT_SUCCESS);
}
[[noreturn]] void webserver_print_error_menu() {
	fprintf(stdout,
			"Error: Incorrect amount of parameters (must be 1).\n"
			"Open the help menu using ./webcrawler --help or ./webcrawl -h\n"
	);
	exit(EXIT_FAILURE);
}
