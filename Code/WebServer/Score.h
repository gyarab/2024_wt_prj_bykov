#ifndef WEBSERVER_FILE_SCORE
#define WEBSERVER_FILE_SCORE
#include "Database.h"

//returns percentage of string likeness 0-1
double_t webserver_string_equality(const char* ars1, const char* ars2);

//under 25% - not a result
#define WEBSERVER_SCORE_MINIMUM (double)3.0

//gets a score of the entry, the higher the score the better (from 0 to slightly more than 1)
double_t webserver_calculate_score(webserver_page_info* apentry, const char* arquery);

#endif
