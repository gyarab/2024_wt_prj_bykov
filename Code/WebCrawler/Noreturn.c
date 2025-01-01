#include "Noreturn.h"

[[noreturn]] void webcrawl_print_help_menu() {
	fprintf(stdout,
			"NPWS WebCrawler\n"
			"(c) Martin Bykov 2024-2025\n"
			"Usage:\n"
			"(1) ./webcrawler <seedfile>\n"
			"(2) ./webcrawler --help\n"
			"    ./webcrawler -h\n"
			"----------\n"
			"Usage 1:\n"
			"Start the web crawler.\n"
			"<seedfile> is a file containing starting addresses (i.e. they are the first to be crawled) separated by newlines\n"
			"Addresses can be in any order. Only 1 address is allowed per line.\n"
			"Seedfile example:\n"
			"address1.tld\n"
			"address2.tld\n"
			"...\n"
			"address3.tld\n"
			"----------\n"
			"Usage 2:\n"
			"Displays this help menu and exits successfully.\n"
			"----------\n"
	);
	exit(EXIT_SUCCESS);
}
[[noreturn]] void webcrawl_print_error_menu() {
	fprintf(stdout,
			"Error: Incorrect amount of parameters (must be 1).\n"
			"Open the help menu using ./webcrawler --help or ./webcrawl -h\n"
	);
	exit(EXIT_FAILURE);
}

//keyboard interrupt handler so we can stop crawler and not destroy the DB
//this data structure exists for passing data to handler (function cannot take more arguments)

struct {
	webcrawl_wid_database* wid;
	webcrawl_drd_database* drd;
	webcrawl_queue* queue;
} webcrawl_keyboard_interrupt_handler_values;

void webcrawl_set_keyboard_interrupt_handler(webcrawl_wid_database* apwid, webcrawl_drd_database* apdrd, webcrawl_queue* apqueue) {
	webcrawl_keyboard_interrupt_handler_values.wid = apwid;
	webcrawl_keyboard_interrupt_handler_values.drd = apdrd;
	webcrawl_keyboard_interrupt_handler_values.queue = apqueue;
}

[[noreturn]] void webcrawl_keyboard_interrupt_handler(int asignal) {
	//close open databases properly!
	fprintf(stdout, "Interrupt recieved, closing databases...");
	webcrawl_wid_close(webcrawl_keyboard_interrupt_handler_values.wid);
	webcrawl_drd_close(webcrawl_keyboard_interrupt_handler_values.drd);
	webcrawl_free_queue(webcrawl_keyboard_interrupt_handler_values.queue);
	fprintf(stdout, "done!\n");
	exit(EXIT_SUCCESS);
}
