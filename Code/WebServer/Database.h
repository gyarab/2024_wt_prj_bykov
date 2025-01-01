#ifndef WEBSERVER_FILE_DATABASE
#define WEBSERVER_FILE_DATABASE
#include "Utils.h"

#define WEBSERVER_DELIMITER_BYTE (char)255

//type copied from webcrawler (removed domain_id, not needed, changed unsigned char to char since we work with chars here)
typedef struct {
	size_t id; 	//ID in WID
	unsigned char* url;

	//fields which are compared to query

	unsigned char** keywords;
	size_t keywords_amount;
	unsigned char** description_words;
	size_t description_words_amount;
	unsigned char* author;
	unsigned char* title;
	unsigned char* header1;

	//fields that arent
	time_t last_visited;
	size_t amount_links_from;
	size_t amount_links_to;
	size_t amount_visitors;
} webserver_page_info;

void webserver_init_page_info(webserver_page_info* const appageinfo);
void webserver_free_page_info(webserver_page_info* const appageinfo);

//Webserver only accesses the WID, the DRD has no useful data for us

typedef struct {
	sqlite3* db;
	sqlite3_stmt* getter; //gets next entry in row
	size_t entryid;
	sqlite3_stmt* updater; //updates visitors value
} webserver_wid_database;

void webserver_wid_open(webserver_wid_database* apdb);

//cycles through each wid entry, if 0 no more entries found
size_t webserver_wid_get_next_entry(webserver_wid_database* apdb, webserver_page_info* apoutput);
void webserver_wid_reset_entry_counter(webserver_wid_database* apdb);

//updates the amount of visitors in entry
void webserver_wid_update_visitors_in_entry(webserver_wid_database* apdb, webserver_page_info* apentry);

void webserver_wid_close(webserver_wid_database* apdb);

#endif
