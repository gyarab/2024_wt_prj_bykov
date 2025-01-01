#ifndef WEBCRAWL_FILE_INCLUDE
#define WEBCRAWL_FILE_INCLUDE

//includes from the C standard library
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdbit.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>

//we use the CURL library for networking
#include <curl/curl.h>

//we use libxml2 to parse HTML and get the address and button tags
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/xmlIO.h>

//we use SQLite for database storage
#include <sqlite3.h>

//macros

#define WEBCRAWL_USER_AGENT "NiepodleglaWyszukiwarka/0.1 (student crawl bot, megapolisplayer.github.io)"
#define WEBCRAWL_USER_AGENT_SHORT "NiepodleglaWyszukiwarka"
#define WEBCRAWL_DELIMITER_BYTE (unsigned char)255

#define WEBCRAWL_MINUTES(x) x*60
#define WEBCRAWL_HOURS(x) x*3600
#define WEBCRAWL_DAYS(x) x*86400
#define WEBCRAWL_MONTHS(x) x*2592000
#define WEBCRAWL_YEARS(x) x*31536000

//revisit every week
#define WEBCRAWL_REVISIT_TIME WEBCRAWL_DAYS(7)

#endif
