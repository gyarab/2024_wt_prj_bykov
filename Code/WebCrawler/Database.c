#include "Database.h"

//takes a pointer to a raw DB object, returns false on error, true otherwise
void internal_optimize_db(sqlite3* apdb) {
	if(sqlite3_exec(apdb,"PRAGMA synchronous = OFF; PRAGMA journal_mode = WAL; PRAGMA temp_store = MEMORY;", NULL, NULL, NULL) != SQLITE_OK) {
		fprintf(stdout, "Unable to optimize database: %s\n", sqlite3_errstr(sqlite3_errcode(apdb)));
		exit(EXIT_FAILURE);
	}
}

//arerrname is the name which is printed in case of error
void internal_precompile_statement(sqlite3* apdb, sqlite3_stmt** appsm, const char* arerrname, const char* arstatement) {
	if(sqlite3_prepare_v2(apdb, arstatement,-1, appsm, NULL) != SQLITE_OK) {
		fprintf(stdout, "Unable to precompile database %s statement: %s\n", arerrname, sqlite3_errstr(sqlite3_errcode(apdb)));
		exit(EXIT_FAILURE);
	}
}

//arerrname is the name which is printed in case of error
void internal_execute_statement(sqlite3* apdb, sqlite3_stmt* apsm, const char* arerrname) {
	if(sqlite3_step(apsm) != SQLITE_DONE) {
		fprintf(stdout, "Unable to insert %s entry: %s\n", arerrname, sqlite3_errstr(sqlite3_errcode(apdb)));
		exit(EXIT_FAILURE);
	}
	if(sqlite3_reset(apsm) != SQLITE_OK) {
		fprintf(stdout, "Unable to reset %s inserter: %s\n", arerrname, sqlite3_errstr(sqlite3_errcode(apdb)));
		exit(EXIT_FAILURE);
	};
	sqlite3_clear_bindings(apsm);
}

//WID

void webcrawl_wid_open(webcrawl_wid_database* apdb) {
	if(sqlite3_open_v2("wcwid.db", &apdb->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL) != SQLITE_OK) {
		fprintf(stdout, "Unable to open WID database\n");
		exit(EXIT_FAILURE);
	}

	if(sqlite3_exec(apdb->db,
		"CREATE TABLE IF NOT EXISTS wid(id integer PRIMARY KEY AUTOINCREMENT, domainid integer, address text UNIQUE, keywords text, descriptions text, authors text, title text, headerone text, lastvisit integer, urlsfrom integer, urlsto integer, visitors integer);"
		"CREATE INDEX IF NOT EXISTS widaddress ON wid(address);"
		"CREATE INDEX IF NOT EXISTS widid ON wid(id);", NULL, NULL, NULL) != SQLITE_OK) {
		fprintf(stdout, "Unable to create WID database: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	}

	//optimize database
	internal_optimize_db(apdb->db);

	//prepare statements

	//inserter
	internal_precompile_statement(apdb->db, &apdb->inserter, "WID inserter", "INSERT OR IGNORE INTO wid(domainid, address, keywords, descriptions, authors, title, headerone, lastvisit, urlsfrom, urlsto, visitors) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
	//domain name getter
	internal_precompile_statement(apdb->db, &apdb->addrgetter, "WID address getter", "SELECT * FROM wid WHERE address = ?;");
	//domain id getter
	internal_precompile_statement(apdb->db, &apdb->idgetter, "WID id getter", "SELECT * FROM wid WHERE domainid = ?;");
	//updater
	internal_precompile_statement(apdb->db, &apdb->updater, "WID updater", "UPDATE wid SET urlsto = ?, lastvisit = ? WHERE id = ?;");
	//deleter
	internal_precompile_statement(apdb->db, &apdb->deleter, "WID deleter", "DELETE FROM wid WHERE id = ?;");
}
size_t webcrawl_wid_add_entry(webcrawl_wid_database* apdb, webcrawl_page_info* apentry) {
	unsigned char* keywords = webcrawl_combine_strings(apentry->keywords, apentry->keywords_amount);
	unsigned char* description = webcrawl_combine_strings(apentry->description_words, apentry->description_words_amount);

	//binding values has first index 1!!!! (reading has 0)
	sqlite3_bind_int(apdb->inserter, 1, apentry->domain_id);
	sqlite3_bind_text(apdb->inserter, 2, (const char*)apentry->url, -1,  SQLITE_TRANSIENT);
	sqlite3_bind_text(apdb->inserter, 3, (const char*)keywords, -1,  SQLITE_TRANSIENT);
	sqlite3_bind_text(apdb->inserter, 4, (const char*)description, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(apdb->inserter, 5, (const char*)apentry->author, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(apdb->inserter, 6, (const char*)apentry->title, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(apdb->inserter, 7, (const char*)apentry->header1, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(apdb->inserter, 8, apentry->last_visited);
	sqlite3_bind_int(apdb->inserter, 9, apentry->amount_links_from);
	sqlite3_bind_int(apdb->inserter, 10, apentry->amount_links_to);
	sqlite3_bind_int(apdb->inserter, 11, apentry->amount_visitors);

	internal_execute_statement(apdb->db, apdb->inserter, "WID inserter");

	free(keywords);
	free(description);

	return sqlite3_last_insert_rowid(apdb->db);
}

size_t webcrawl_wid_get_entry_by_address(webcrawl_wid_database* apdb, const unsigned char* araddress, webcrawl_page_info* apoutput) {
	sqlite3_bind_text(apdb->addrgetter, 1, (const char*)araddress, -1,  SQLITE_TRANSIENT);

	//do not init or free here, apoutput might be stack allocated!

	size_t amount_rows_returned = 0;
	int sql_result = sqlite3_step(apdb->addrgetter);
	switch(sql_result) {
		case(SQLITE_ROW):
			apoutput->id = sqlite3_column_int64(apdb->addrgetter, 0);
			apoutput->domain_id = sqlite3_column_int64(apdb->addrgetter, 1);

			const unsigned char* str = sqlite3_column_text(apdb->addrgetter, 2);
			size_t str_length = 0;
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->url = malloc((str_length+1));
				memcpy(apoutput->url, str, strlen((const char*)str));
				apoutput->url[str_length] = '\0';
			}

			str = sqlite3_column_text(apdb->addrgetter, 3);
			if(str != NULL) {
				webcrawl_split_ustr(str, WEBCRAWL_DELIMITER_BYTE, &apoutput->keywords, &apoutput->keywords_amount, 0);
			}

			str = sqlite3_column_text(apdb->addrgetter, 4);
			if(str != NULL) {
				webcrawl_split_ustr(str, WEBCRAWL_DELIMITER_BYTE, &apoutput->description_words, &apoutput->description_words_amount, 0);
			}

			str = sqlite3_column_text(apdb->addrgetter, 5);
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->author = malloc((str_length+1));
				memcpy(apoutput->author, str, str_length);
				apoutput->author[str_length] = '\0';
			}

			str = sqlite3_column_text(apdb->addrgetter, 6);
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->title = malloc((str_length+1));
				memcpy(apoutput->title, str, str_length);
				apoutput->title[str_length] = '\0';
			}

			str = sqlite3_column_text(apdb->addrgetter, 7);
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->header1 = malloc((str_length+1));
				memcpy(apoutput->header1, str, str_length);
				apoutput->header1[str_length] = '\0';
			}

			apoutput->last_visited = sqlite3_column_int64(apdb->addrgetter, 8);
			apoutput->amount_links_from = sqlite3_column_int64(apdb->addrgetter, 9);
			apoutput->amount_links_to = sqlite3_column_int64(apdb->addrgetter, 10);
			apoutput->amount_visitors = sqlite3_column_int64(apdb->addrgetter, 11);

			amount_rows_returned = 1;
			break;
		case(SQLITE_DONE):
			break;
		default:
			fprintf(stdout, "Unable to get WID entry: %s (code %i)\n", sqlite3_errstr(sqlite3_errcode(apdb->db)), sql_result);
			exit(EXIT_FAILURE);
	}

	if(sqlite3_reset(apdb->addrgetter) != SQLITE_OK) {
		fprintf(stdout, "Unable to reset WID addrgetter: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	};

	sqlite3_clear_bindings(apdb->addrgetter);
	return amount_rows_returned;
}
void webcrawl_wid_get_entry_by_domain_id(webcrawl_wid_database* apdb, const size_t adomainid, webcrawl_page_info** apoutput, size_t* aamountout) {
	sqlite3_bind_int64(apdb->idgetter, 1, adomainid);

	*apoutput = NULL;
	*aamountout = 0;

	//getting values has first index 0!!!!!
	int sql_result = -1;
	while(sql_result != SQLITE_DONE) {
		sql_result = sqlite3_step(apdb->idgetter);
		switch(sql_result) {
			case(SQLITE_ROW):
				(*aamountout)++;
				apoutput = realloc(apoutput, sizeof(unsigned char*)*(*aamountout));
				apoutput[(*aamountout)-1] = malloc(sizeof(webcrawl_page_info));
				webcrawl_init_page_info(apoutput[(*aamountout)-1]);

				apoutput[(*aamountout)-1]->id = sqlite3_column_int64(apdb->idgetter, 0);
				apoutput[(*aamountout)-1]->domain_id = sqlite3_column_int64(apdb->idgetter, 1);

				const unsigned char* str = sqlite3_column_text(apdb->idgetter, 2);
				size_t str_length = 0;
				if(str != NULL) {
					str_length = strlen((const char*)str);
					apoutput[(*aamountout)-1]->url = malloc(str_length);
					memcpy(apoutput[(*aamountout)-1]->url, str, strlen((const char*)str));
				}

				str = sqlite3_column_text(apdb->idgetter, 3);
				webcrawl_split_ustr(str, WEBCRAWL_DELIMITER_BYTE, &apoutput[(*aamountout)-1]->keywords, &apoutput[(*aamountout)-1]->keywords_amount, 0);

				str = sqlite3_column_text(apdb->idgetter, 4);
				webcrawl_split_ustr(str, WEBCRAWL_DELIMITER_BYTE, &apoutput[(*aamountout)-1]->description_words, &apoutput[(*aamountout)-1]->description_words_amount, 0);

				str = sqlite3_column_text(apdb->idgetter, 5);
				if(str != NULL) {
					str_length = strlen((const char*)str);
					apoutput[(*aamountout)-1]->author = malloc(str_length);
					memcpy(apoutput[(*aamountout)-1]->author, str, str_length);
				}

				str = sqlite3_column_text(apdb->idgetter, 6);
				if(str != NULL) {
					str_length = strlen((const char*)str);
					apoutput[(*aamountout)-1]->title = malloc(str_length);
					memcpy(apoutput[(*aamountout)-1]->title, str, str_length);
				}

				str = sqlite3_column_text(apdb->idgetter, 7);
				if(str != NULL) {
					str_length = strlen((const char*)str);
					apoutput[(*aamountout)-1]->header1 = malloc(str_length);
					memcpy(apoutput[(*aamountout)-1]->header1, str, str_length);
				}

				apoutput[(*aamountout)-1]->last_visited = sqlite3_column_int64(apdb->idgetter, 8);
				apoutput[(*aamountout)-1]->amount_links_from = sqlite3_column_int64(apdb->idgetter, 9);
				apoutput[(*aamountout)-1]->amount_links_to = sqlite3_column_int64(apdb->idgetter, 10);
				apoutput[(*aamountout)-1]->amount_visitors = sqlite3_column_int64(apdb->idgetter, 11);
				break;
			case(SQLITE_DONE):
				break; //no more data, just skip
			default:
				fprintf(stdout, "Unable to get WID entry: %s (code %i)\n", sqlite3_errstr(sqlite3_errcode(apdb->db)), sql_result);
				exit(EXIT_FAILURE);
		}
	}

	if(sqlite3_reset(apdb->idgetter) != SQLITE_OK) {
		fprintf(stdout, "Unable to reset WID idgetter: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	};

	sqlite3_clear_bindings(apdb->idgetter);
}

void webcrawl_wid_update_entry(webcrawl_wid_database* apdb,  webcrawl_page_info* apentry) {
	sqlite3_bind_int(apdb->updater, 1, apentry->amount_links_to);
	sqlite3_bind_int(apdb->updater, 2, apentry->last_visited);
	sqlite3_bind_int(apdb->updater, 3, apentry->id);
	internal_execute_statement(apdb->db, apdb->updater, "WID updater");
}

void webcrawl_wid_delete_element(webcrawl_wid_database* apdb,  webcrawl_page_info* apentry) {
	sqlite3_bind_int64(apdb->deleter, 1, apentry->id);
	internal_execute_statement(apdb->db, apdb->deleter, "WID deleter");
}

void webcrawl_free_wid_entries(webcrawl_page_info* apentries, const size_t asize) {
	for(size_t n = 0; n < asize; n++) {
		webcrawl_free_page_info(&apentries[n]);
	}
	free(apentries);
}

void webcrawl_wid_close(webcrawl_wid_database* apdb) {
	sqlite3_finalize(apdb->inserter);
	sqlite3_finalize(apdb->addrgetter);
	sqlite3_finalize(apdb->idgetter);
	sqlite3_finalize(apdb->updater);
	sqlite3_finalize(apdb->deleter);
	sqlite3_close(apdb->db);
}

//DRD

void webcrawl_init_drd_entry(webcrawl_drd_entry* apentry) {
	apentry->domainname = NULL;
	apentry->pages_amount = 0;
	apentry->allowed = NULL;
	apentry->amount_allowed = 0;
	apentry->disallowed = NULL;
	apentry->amount_disallowed = 0;
	apentry->seconds_between_requests = 0;
	apentry->last_crawled = 0;
}
void webcrawl_free_drd_entry(webcrawl_drd_entry* apentry) {
	free(apentry->domainname);
	apentry->pages_amount = 0;

	for(size_t n = 0; n < apentry->amount_allowed; n++) {
		free(apentry->allowed[n]);
	}
	free(apentry->allowed);
	apentry->amount_allowed = 0;

	for(size_t n = 0; n < apentry->amount_disallowed; n++) {
		free(apentry->disallowed[n]);
	}
	free(apentry->disallowed);
	apentry->amount_disallowed = 0;

	apentry->seconds_between_requests = 0;
	apentry->last_crawled = 0;
}

void webcrawl_drd_open(webcrawl_drd_database* apdb) {
	if(sqlite3_open("wcdrd.db", &apdb->db) != SQLITE_OK) {
		fprintf(stdout, "Unable to open DRD database\n");
		exit(EXIT_FAILURE);
	}
	if(sqlite3_exec(apdb->db,
		"CREATE TABLE IF NOT EXISTS drd(domainid integer PRIMARY KEY AUTOINCREMENT, name text UNIQUE, pages integer, allow text, disallow text, crawltime integer, lastcrawled integer);"
		"CREATE INDEX IF NOT EXISTS drdname ON drd(name);", NULL, NULL, NULL) != SQLITE_OK) {
		fprintf(stdout, "Unable to create DRD database: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	}

	//optimize database
	internal_optimize_db(apdb->db);

	//prepare statements
	if(sqlite3_prepare_v2(apdb->db,
		"INSERT OR IGNORE INTO drd(name, pages, allow, disallow, crawltime, lastcrawled) VALUES(?, ?, ?, ?, ?, ?);",
		-1, &apdb->inserter, NULL) != SQLITE_OK) {
		fprintf(stdout, "Unable to precompile DRD database inserter statement: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	}

	internal_precompile_statement(apdb->db, &apdb->getter, "DRD getter", "SELECT * FROM drd WHERE name = ?;");
	internal_precompile_statement(apdb->db, &apdb->lcgetter, "DRD lcgetter", "SELECT lastcrawled FROM drd WHERE name = ?;");
	internal_precompile_statement(apdb->db, &apdb->didgetter, "DRD didgetter", "SELECT domainid FROM drd WHERE name = ?;");
	internal_precompile_statement(apdb->db, &apdb->updater, "DRD updater", "UPDATE drd SET pages = ?, lastcrawled = ? WHERE name = ?;");
}
void webcrawl_drd_add_entry(webcrawl_drd_database* apdb, webcrawl_drd_entry* apentry) {
	//binding values has first index 1!!!! (reading has 0)
	sqlite3_bind_text(apdb->inserter, 1, (const char*)apentry->domainname, -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(apdb->inserter, 2, apentry->pages_amount);

	unsigned char* combined_allowed = webcrawl_combine_strings(apentry->allowed, apentry->amount_allowed);
	sqlite3_bind_text(apdb->inserter, 3, (const char*)combined_allowed, -1,  SQLITE_TRANSIENT);
	unsigned char* combined_disallowed = webcrawl_combine_strings(apentry->disallowed, apentry->amount_disallowed);
	sqlite3_bind_text(apdb->inserter, 4, (const char*)combined_disallowed, -1,  SQLITE_TRANSIENT);

	sqlite3_bind_int(apdb->inserter, 5, apentry->seconds_between_requests);
	sqlite3_bind_int(apdb->inserter, 6, apentry->last_crawled);

	internal_execute_statement(apdb->db, apdb->inserter, "DRD inserter");

	free(combined_allowed);
	free(combined_disallowed);
}

size_t webcrawl_drd_get_entry_by_name(webcrawl_drd_database* apdb, const unsigned char* arstring, webcrawl_drd_entry* apoutput) {
	sqlite3_bind_text(apdb->getter, 1, (char*)arstring, -1, SQLITE_TRANSIENT);

	//getting values has first index 0!!!!! (binding has 1!!!)
	size_t amount_rows_returned = 0;
	switch(sqlite3_step(apdb->getter)) {
		case(SQLITE_ROW):
			apoutput->domainid = sqlite3_column_int64(apdb->getter, 0);
			const unsigned char* str = sqlite3_column_text(apdb->getter, 1);
			size_t str_length = 0;
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->domainname = malloc(str_length+1);
				memcpy(apoutput->domainname, str, str_length);
				apoutput->domainname[str_length] = '\0';
			}
			apoutput->pages_amount = sqlite3_column_int64(apdb->getter, 2);
			webcrawl_split_ustr(sqlite3_column_text(apdb->getter, 3), WEBCRAWL_DELIMITER_BYTE, &apoutput->allowed, &apoutput->amount_allowed, 0);
			webcrawl_split_ustr(sqlite3_column_text(apdb->getter, 4), WEBCRAWL_DELIMITER_BYTE, &apoutput->disallowed, &apoutput->amount_disallowed, 0);
			apoutput->seconds_between_requests = sqlite3_column_int64(apdb->getter, 5);
			apoutput->last_crawled = sqlite3_column_int64(apdb->getter, 6);
			amount_rows_returned = 1;
			break;
		case(SQLITE_DONE):
			break;
		default:
			fprintf(stdout, "Unable to get DRD entry: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
			exit(EXIT_FAILURE);
	}

	if(sqlite3_reset(apdb->getter) != SQLITE_OK) {
		fprintf(stdout, "Unable to reset DRD getter: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	};
	sqlite3_clear_bindings(apdb->getter);

	return amount_rows_returned;
}

size_t internal_drd_execute_one_int_statement(sqlite3* apdb, sqlite3_stmt* apstatement) {
	size_t return_value = SIZE_MAX;
	//getting values has first index 0!!!!!
	switch(sqlite3_step(apstatement)) {
		case(SQLITE_ROW):
			return_value = sqlite3_column_int64(apstatement, 0);
			break;
		case(SQLITE_DONE):
			break;
		default:
			fprintf(stdout, "Unable to get DRD entry: %s\n", sqlite3_errstr(sqlite3_errcode(apdb)));
			exit(EXIT_FAILURE);
	}

	if(sqlite3_reset(apstatement) != SQLITE_OK) {
		fprintf(stdout, "Unable to reset DRD statement: %s\n", sqlite3_errstr(sqlite3_errcode(apdb)));
		exit(EXIT_FAILURE);
	};
	sqlite3_clear_bindings(apstatement);

	return return_value;
}

size_t webcrawl_drd_get_last_crawled_by_name(webcrawl_drd_database* apdb, const unsigned char* arstring) {
	sqlite3_bind_text(apdb->lcgetter, 1, (char*)arstring, -1, SQLITE_TRANSIENT);
	return internal_drd_execute_one_int_statement(apdb->db, apdb->lcgetter);
}
size_t webcrawl_drd_get_domain_id_by_name(webcrawl_drd_database* apdb, const unsigned char* arstring) {
	sqlite3_bind_text(apdb->didgetter, 1, (char*)arstring, -1, SQLITE_TRANSIENT);
	return internal_drd_execute_one_int_statement(apdb->db, apdb->didgetter);
}

void webcrawl_drd_update_entry(webcrawl_drd_database* apdb,  webcrawl_drd_entry* apentry) {
	sqlite3_bind_int(apdb->updater, 1, apentry->pages_amount);
	sqlite3_bind_int(apdb->updater, 2, apentry->last_crawled);
	sqlite3_bind_text(apdb->updater, 3, (char*)apentry->domainname, -1, SQLITE_TRANSIENT);
	internal_execute_statement(apdb->db, apdb->updater, "DRD updater");
}

void webcrawl_drd_close(webcrawl_drd_database* apdb) {
	sqlite3_finalize(apdb->inserter);
	sqlite3_finalize(apdb->getter);
	sqlite3_finalize(apdb->lcgetter);
	sqlite3_finalize(apdb->didgetter);
	sqlite3_finalize(apdb->updater);
	sqlite3_close(apdb->db);
}
