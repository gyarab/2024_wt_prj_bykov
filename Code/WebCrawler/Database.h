#ifndef WEBCRAWL_FILE_DATABASE
#define WEBCRAWL_FILE_DATABASE
#include "Types.h"

//when checking hits: if not found, create entry - if found, update score
//2 database:

//Domain Rules Database (DRD)
//DOMAIN ID | DISALLOWED_SUBADDRESSES
// 1        | /personal/,/norobot*/      // use function to match (including wildcards)

//Website Indexing Database (WID)
//SITE ID | DOMAIN ID | KEYWORDS | DESCRIPTIONS |
//AUTHORS | TITLE | HEADER1 | LAST_VISITED | FROM | TO | VISITORS | PRECALC

//we will compare the query to the text things anyway anyway
//generate a percentage for each text field (or all together), then multiply/add and sort by it

//WID (Website Indexing Database) - the main one

typedef struct {
	sqlite3* db;
	sqlite3_stmt* inserter;
	sqlite3_stmt* addrgetter; //get by address
	sqlite3_stmt* idgetter; //get by domain id
	sqlite3_stmt* updater; //updates values
	sqlite3_stmt* deleter; //deletes entry

	//query stuff implemented in WebServer
} webcrawl_wid_database;

typedef struct {
	webcrawl_page_info* entries;
	size_t entries_amount;
} webcrawl_wid_results;

void webcrawl_wid_open(webcrawl_wid_database* apdb);

//returns the primary key of the new value
size_t webcrawl_wid_add_entry(webcrawl_wid_database* apdb, webcrawl_page_info* apentry);

//get entry functions return array, free using webcrawl_free_wid_entries (if array size 0 and pointer is NULL no entries have been returned)
//NO NEED TO INIT ENTRIES BEFORE CALLING FUNCTION

//will return either 1 if found, or 0 if no results, address is unique
size_t webcrawl_wid_get_entry_by_address(webcrawl_wid_database* apdb, const unsigned char* araddress, webcrawl_page_info* apoutput);
void webcrawl_wid_get_entry_by_domain_id(webcrawl_wid_database* apdb, const size_t adomainid, webcrawl_page_info** apoutput, size_t* aamountout);

//updates WID database entry (update entry where the the address is entry.url)
void webcrawl_wid_update_entry(webcrawl_wid_database* apdb,  webcrawl_page_info* apentry);

//deletes WID entry, does NOT free page info
void webcrawl_wid_delete_element(webcrawl_wid_database* apdb,  webcrawl_page_info* apentry);

void webcrawl_free_wid_entries(webcrawl_page_info* apentries, const size_t asize);

void webcrawl_wid_close(webcrawl_wid_database* apdb);

//DRD (Domain Rules Database) - the robots txt one

typedef struct {
	sqlite3* db;
	sqlite3_stmt* inserter;
	sqlite3_stmt* getter;
	sqlite3_stmt* lcgetter;
	sqlite3_stmt* didgetter;
	sqlite3_stmt* updater;
} webcrawl_drd_database;

typedef struct {
	size_t domainid;
	unsigned char* domainname;
	size_t pages_amount;

	unsigned char** allowed;
	size_t amount_allowed;

	unsigned char** disallowed;
	size_t amount_disallowed;

	size_t seconds_between_requests;
	size_t last_crawled;
} webcrawl_drd_entry;

void webcrawl_init_drd_entry(webcrawl_drd_entry* apentry);
void webcrawl_free_drd_entry(webcrawl_drd_entry* apentry);

void webcrawl_drd_open(webcrawl_drd_database* apdb);
void webcrawl_drd_add_entry(webcrawl_drd_database* apdb, webcrawl_drd_entry* apentry);

//NO NEED TO INIT ENTRIES BEFORE CALLING FUNCTION

//will return either 1 if found, or 0 if no results, domain id is unique
size_t webcrawl_drd_get_entry_by_name(webcrawl_drd_database* apdb, const unsigned char* arstring, webcrawl_drd_entry* apoutput);
//returns time as unix timestamp, returns 0 on failure
size_t webcrawl_drd_get_last_crawled_by_name(webcrawl_drd_database* apdb, const unsigned char* arstring);
//returns domain id, SIZE_MAX on failure
size_t webcrawl_drd_get_domain_id_by_name(webcrawl_drd_database* apdb, const unsigned char* arstring);

//updates DRD database entry (update entry where the the domain name is entry.domainname)
void webcrawl_drd_update_entry(webcrawl_drd_database* apdb,  webcrawl_drd_entry* apentry);

void webcrawl_drd_close(webcrawl_drd_database* apdb);

#endif
