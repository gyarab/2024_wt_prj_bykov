#ifndef WEBSERVER_FILE_SERIALIZATION
#define WEBSERVER_FILE_SERIALIZATION
#include "Score.h"

//we dont care about signed/unsigned, this is binary data

//1
#define WEBSERVER_ENTRY_NEW   (unsigned char)0b00000001
//2
#define WEBSERVER_ENTRY_SPLIT (unsigned char)0b00000010

//name(2)author(2)address(2)description(1)...

//sorting and score calculation happens here, in webserver

//free returned pointer!
char* webserver_serialize_query_result(webserver_page_info* apinfo);

#endif
