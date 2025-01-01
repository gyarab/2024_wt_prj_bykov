#include "Socketlib.h"

//apfreepiN - point to page info that needs freeing (variadic arguments overkill for such an app)
void webserver_set_interrupt_handler(webserver_socket asock, webserver_wid_database* apwid, webserver_page_info* apfreepi1);

//readsock setting function separate, readsock changes often
void webserver_set_interrupt_handler_readsock(webserver_socket areadsock);
//readsock setting function separate, pageinfo changes often
void webserver_set_interrupt_handler_pi(webserver_page_info* apfreepi1);

//interrupt handler closes sockets, terminates library, frees buffer and exits
[[noreturn]] void webserver_keyboard_interrupt_handler(int asignal);

//prints help menu and such, same as webcrawler
[[noreturn]] void webserver_print_help_menu();
[[noreturn]] void webserver_print_error_menu();
