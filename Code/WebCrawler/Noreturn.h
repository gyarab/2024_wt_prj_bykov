#ifndef WEBCRAWL_FILE_NORETURN
#define WEBCRAWL_FILE_NORETURN
#include "Processing.h"

//these functions print a menu and exit the program
[[noreturn]] void webcrawl_print_help_menu();
[[noreturn]] void webcrawl_print_error_menu();

//set database pointers to interrupt handler
void webcrawl_set_keyboard_interrupt_handler(webcrawl_wid_database* apwid, webcrawl_drd_database* apdrd, webcrawl_queue* apqueue);

//function called when user presses Ctrl+C (SIGINT) - saves databases and exits
[[noreturn]] void webcrawl_keyboard_interrupt_handler(int asignal);

#endif
