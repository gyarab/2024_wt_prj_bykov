#ifndef WEBSERVER_FILE_UTILS
#define WEBSERVER_FILE_UTILS
#include "Include.h"

void webserver_split_ustr(const unsigned char* arstring, const unsigned char ardelimiter, unsigned char*** apprstringarray, size_t* aarraysize, size_t amaxsize);
void webserver_free_ustr_array(unsigned char** aprstringarray, const size_t aarraysize);

#endif
