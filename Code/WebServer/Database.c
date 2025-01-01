//large part of this file is copied from the WebCrawler DB library with some small changes and omissions
#include "Database.h"

void webserver_init_page_info(webserver_page_info* const appageinfo) {
	appageinfo->id = SIZE_MAX;
	appageinfo->url = NULL;
	appageinfo->keywords = NULL;
	appageinfo->keywords_amount = 0;
	appageinfo->description_words = NULL;
	appageinfo->description_words_amount = 0;
	appageinfo->author = NULL;
	appageinfo->title = NULL;
	appageinfo->header1 = NULL;
	appageinfo->last_visited = 0;
	appageinfo->amount_links_from = 0;
	appageinfo->amount_links_to = 0;
	appageinfo->amount_visitors = 0;
}
void webserver_free_page_info(webserver_page_info* const appageinfo) {
	if(appageinfo == NULL) return;

	appageinfo->id = 0;

	free(appageinfo->url);

	for(size_t n = 0; n < appageinfo->keywords_amount; n++) {
		free(appageinfo->keywords[n]);
	}
	free(appageinfo->keywords);
	appageinfo->keywords = NULL;
	appageinfo->keywords_amount = 0;

	for(size_t n = 0; n < appageinfo->description_words_amount; n++) {
		free(appageinfo->description_words[n]);
	}
	free(appageinfo->description_words);
	appageinfo->description_words = NULL;
	appageinfo->description_words_amount = 0;

	free(appageinfo->author);
	appageinfo->author = NULL;

	free(appageinfo->title);
	appageinfo->title = NULL;
	free(appageinfo->header1);
	appageinfo->header1 = NULL;

	appageinfo->last_visited = 0;
	appageinfo->amount_links_from = 0;
	appageinfo->amount_links_to = 0;
	appageinfo->amount_visitors = 0;
}

//arerrname is the name which is printed in case of error
void internal_precompile_statement(sqlite3* apdb, sqlite3_stmt** appsm, const char* arerrname, const char* arstatement) {
	if(sqlite3_prepare_v2(apdb, arstatement,-1, appsm, NULL) != SQLITE_OK) {
		fprintf(stdout, "Unable to precompile database %s statement: %s\n", arerrname, sqlite3_errstr(sqlite3_errcode(apdb)));
		exit(EXIT_FAILURE);
	}
}

void webserver_wid_open(webserver_wid_database* apdb) {
	if(sqlite3_open_v2("wcwid.db", &apdb->db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, NULL) != SQLITE_OK) {
		fprintf(stdout, "Unable to open WID database\n");
		exit(EXIT_FAILURE);
	}

	//getter
	internal_precompile_statement(apdb->db, &apdb->getter, "WID getter", "SELECT * FROM wid WHERE id = ?;");
	//updater - updates
	internal_precompile_statement(apdb->db, &apdb->updater, "WID updater", "UPDATE wid SET visitors = ? WHERE id = ?;");

	apdb->entryid = 1;
}
size_t webserver_wid_get_next_entry(webserver_wid_database* apdb, webserver_page_info* apoutput) {
	sqlite3_bind_int64(apdb->getter, 1, apdb->entryid);
	apdb->entryid++;

	size_t amount_rows_returned = 0;
	int sql_result = sqlite3_step(apdb->getter);
	switch(sql_result) {
		case(SQLITE_ROW):
			apoutput->id = sqlite3_column_int64(apdb->getter, 0);
			//ignore domain_id at ID 1

			const unsigned char* str = sqlite3_column_text(apdb->getter, 2);
			size_t str_length = 0;
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->url = malloc((str_length+1));
				memcpy(apoutput->url, str, strlen((const char*)str));
				apoutput->url[str_length] = '\0';
			}

			str = sqlite3_column_text(apdb->getter, 3);
			if(str != NULL) {
				webserver_split_ustr(str, WEBSERVER_DELIMITER_BYTE, &apoutput->keywords, &apoutput->keywords_amount, 0);
			}

			str = sqlite3_column_text(apdb->getter, 4);
			if(str != NULL) {
				webserver_split_ustr(str, WEBSERVER_DELIMITER_BYTE, &apoutput->description_words, &apoutput->description_words_amount, 0);
			}

			str = sqlite3_column_text(apdb->getter, 5);
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->author = malloc((str_length+1));
				memcpy(apoutput->author, str, str_length);
				apoutput->author[str_length] = '\0';
			}

			str = sqlite3_column_text(apdb->getter, 6);
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->title = malloc((str_length+1));
				memcpy(apoutput->title, str, str_length);
				apoutput->title[str_length] = '\0';
			}

			str = sqlite3_column_text(apdb->getter, 7);
			if(str != NULL) {
				str_length = strlen((const char*)str);
				apoutput->header1 = malloc((str_length+1));
				memcpy(apoutput->header1, str, str_length);
				apoutput->header1[str_length] = '\0';
			}

			apoutput->last_visited = sqlite3_column_int64(apdb->getter, 8);
			apoutput->amount_links_from = sqlite3_column_int64(apdb->getter, 9);
			apoutput->amount_links_to = sqlite3_column_int64(apdb->getter, 10);
			apoutput->amount_visitors = sqlite3_column_int64(apdb->getter, 11);

			amount_rows_returned = 1;
			break;
		case(SQLITE_DONE):
			break;
		default:
			fprintf(stdout, "Unable to get WID entry: %s (code %i)\n", sqlite3_errstr(sqlite3_errcode(apdb->db)), sql_result);
			exit(EXIT_FAILURE);
	}

	if(sqlite3_reset(apdb->getter) != SQLITE_OK) {
		fprintf(stdout, "Unable to reset WID getter: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	};

	sqlite3_clear_bindings(apdb->getter);
	return amount_rows_returned;
}
void webserver_wid_reset_entry_counter(webserver_wid_database* apdb) {
	apdb->entryid = 1; //id starts at 1
}
void webserver_wid_update_visitors_in_entry(webserver_wid_database* apdb, webserver_page_info* apentry) {
	sqlite3_bind_int64(apdb->updater, 1, apentry->id);
	sqlite3_bind_int64(apdb->updater, 2, apentry->amount_visitors);

	if(sqlite3_step(apdb->updater) != SQLITE_DONE) {
		fprintf(stdout, "Unable to update WID entry: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	}
	if(sqlite3_reset(apdb->updater) != SQLITE_OK) {
		fprintf(stdout, "Unable to reset WID updater: %s\n", sqlite3_errstr(sqlite3_errcode(apdb->db)));
		exit(EXIT_FAILURE);
	};
	sqlite3_clear_bindings(apdb->updater);
}
void webserver_wid_close(webserver_wid_database* apdb) {
	sqlite3_finalize(apdb->getter);
	sqlite3_finalize(apdb->updater);
	sqlite3_close(apdb->db);
}
