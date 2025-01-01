#include "System.h"

#define WEBSERVER_AMOUNT_OF_RESULTS 10
#define WEBSERVER_QUERY_BUFFER_SIZE 4000
#define WEBSERVER_RESPONSE_BUFFER_SIZE 16000

int main(int aargumentamount, char** aprarguments) {
	signal(SIGINT, webserver_keyboard_interrupt_handler); //on keyboard exit

	//check for correct number of arguments, print menus accordingly
	if(aargumentamount != 2) webserver_print_error_menu();
	if(strcmp(aprarguments[1], "--help") == 0 || strcmp(aprarguments[1], "-h") == 0) webserver_print_help_menu();

	fprintf(stdout, "NPWS WebServer\n(c) Martin Bykov 2024-2025\n");

	//init libraries and variables

	webserver_wid_database wid; //database
	webserver_wid_open(&wid);

	webserver_page_info* pageinfobuffer = malloc(sizeof(webserver_page_info)); //buffer to save current row
	memset(pageinfobuffer, 0, sizeof(webserver_page_info));
	webserver_page_info* bestresults = malloc(sizeof(webserver_page_info)*WEBSERVER_AMOUNT_OF_RESULTS);
	memset(bestresults, 0, sizeof(webserver_page_info)*WEBSERVER_AMOUNT_OF_RESULTS);
	double resultscores[WEBSERVER_AMOUNT_OF_RESULTS];
	memset(resultscores, 0, sizeof(uint64_t)*WEBSERVER_AMOUNT_OF_RESULTS);

	double_t pageinfobestscore = 0.0; //score of best result
	double_t pageinfoworstscore = 0.0; //score of worst result which will get sent

	webserver_socketlib_init(aprarguments[1]);
	webserver_socket s = webserver_socketlib_open();
	webserver_socket rs;
	webserver_set_interrupt_handler(s, &wid, pageinfobuffer); //the handler closes the socket, saves the DB and frees some stuff

	char* header = "HTTP/1.1 200 OK\r\nAccept-Encoding: identity\r\nContent-Type: application/octet-stream\r\nAccess-Control-Allow-Origin: *\r\n";
	char* errorheader = "HTTP/1.1 204 Bad Request\r\nAccept-Encoding: identity\r\nAccess-Control-Allow-Origin: *\r\n";
	char* contentlen = "Content-Length: %zu\r\n\r\n";

	char* response;

	char buffer[WEBSERVER_QUERY_BUFFER_SIZE]; //data contains only header and query
	memset(buffer, 0, WEBSERVER_QUERY_BUFFER_SIZE);
	char responsebuffer[WEBSERVER_RESPONSE_BUFFER_SIZE]; //our response data, including header
	memset(responsebuffer, 0, WEBSERVER_RESPONSE_BUFFER_SIZE);

	size_t read = 0;

	char* query = NULL;

	while(true) {
		memset(buffer, 0, WEBSERVER_QUERY_BUFFER_SIZE);
		fprintf(stdout, "Waiting for request...\n");
		rs = webserver_socketlib_wait_for_connection(&s);
		webserver_set_interrupt_handler_readsock(rs);
		if(!webserver_socketlib_listen(rs, buffer, &read, WEBSERVER_QUERY_BUFFER_SIZE-1)) {
			fprintf(stdout, "Invalid connection!\n");
			webserver_socketlib_disconnect(rs);
			continue;
		}
		fprintf(stdout, "Recieved a request!\n");

		//get content (query) from HTTP request, ALWAYS split by empty line
		char* query = strstr(buffer, "\r\n\r\n") + 4; // + length of "needle" string, strstr returns ptr to beginning
		if(query == (char*)0x4 || strlen(query) == 0) {
			//empty query
			fprintf(stdout, "Empty query: returning code 204!\n");
			sprintf(responsebuffer, "%s", errorheader);
			webserver_socketlib_send(rs, responsebuffer, strlen(responsebuffer));
			webserver_socketlib_close(rs);
			continue;
		}
		query = strdup(query);
		fprintf(stdout, "Query: %s \n", query);

		//TODO optimize if becomes problem: load some results into memory, remember most popular?

		fprintf(stdout, "Searching...");

		size_t next_entry = 1;
		//get result for query
		webserver_set_interrupt_handler_pi(pageinfobuffer); //deregister from handler
		while(true) {
			webserver_init_page_info(pageinfobuffer);

			//get entry
			next_entry = webserver_wid_get_next_entry(&wid, pageinfobuffer);
			if(next_entry == 0) {
				//if no entry found (last entry), leave
				webserver_free_page_info(pageinfobuffer);
				break;
			}

			//get score
			double_t score = webserver_calculate_score(pageinfobuffer, query);
			if(score > pageinfobestscore) {
				//better than the best, insert at beginning
				webserver_free_page_info(&bestresults[WEBSERVER_AMOUNT_OF_RESULTS-1]); //free last element
				memmove(&bestresults[1], bestresults, sizeof(webserver_page_info)*(WEBSERVER_AMOUNT_OF_RESULTS-1)); //shift by 1
				memcpy(bestresults, pageinfobuffer, sizeof(webserver_page_info)); //copy to first element
				resultscores[0] = score;
				pageinfobestscore = score;
			}
			else if(score > pageinfoworstscore) {
				//search where to insert
				size_t futureid = WEBSERVER_AMOUNT_OF_RESULTS-1; //max id
				for(size_t i = WEBSERVER_AMOUNT_OF_RESULTS-1; i >= 1; i--) {
					if(score < resultscores[i]) {
						futureid = i - 1;
						break;
					}
				}
				webserver_free_page_info(&bestresults[WEBSERVER_AMOUNT_OF_RESULTS-1]); //free last element
				memmove(&bestresults[futureid+1], &bestresults[futureid], sizeof(webserver_page_info)*(WEBSERVER_AMOUNT_OF_RESULTS-1-futureid)); //shift by 1 (make space for elem)
				memcpy(&bestresults[futureid], pageinfobuffer, sizeof(webserver_page_info)); //copy
				resultscores[futureid] = score;
				pageinfoworstscore = resultscores[WEBSERVER_AMOUNT_OF_RESULTS-10];
			}
			else {
				//if same or worse - just free current buffer
				webserver_free_page_info(pageinfobuffer);
			}
		}
		webserver_set_interrupt_handler_pi(NULL);
		webserver_wid_reset_entry_counter(&wid);
		fprintf(stdout, "done!\n");

		//make response

		memset(responsebuffer, 0, WEBSERVER_RESPONSE_BUFFER_SIZE);
		if(pageinfobestscore < WEBSERVER_SCORE_MINIMUM) {
			response = "NORESULT"; //write response
			size_t amount_written = sprintf(responsebuffer, "%s", header); //write base of header
			snprintf(&responsebuffer[amount_written], strlen(contentlen), contentlen, strlen((char*)response)); //write content length
			sprintf(&responsebuffer[amount_written], "%s", response); //write response

			fprintf(stdout, "Best result score %f lower than minimum (%f)\n", pageinfobestscore, WEBSERVER_SCORE_MINIMUM);
		}
		else {
			fprintf(stdout, "Best result score: %f, worst: %f\nResponding...", pageinfobestscore, pageinfoworstscore);

			//serialize the results
			size_t length_of_response = 0;
			for(size_t i = 0; i < WEBSERVER_AMOUNT_OF_RESULTS; i++) {
				if(!bestresults[i].url) continue; //ignore non-existing results
				if(resultscores[i] <= WEBSERVER_SCORE_MINIMUM) continue; //ignore useless results

				fprintf(stdout, "- %f ", resultscores[i]);
				response = webserver_serialize_query_result(&bestresults[i]);
				length_of_response += sprintf(&responsebuffer[length_of_response], "%s", response); //write response to buffer
				free(response);
				if(length_of_response > WEBSERVER_RESPONSE_BUFFER_SIZE) {
					fprintf(stdout, "Serialized data too large!\n");
					exit(EXIT_FAILURE);
				}
			}
			length_of_response = strlen(responsebuffer); //get actual size of response
			fprintf(stdout, "length %zu...", length_of_response);

			//copy and write to buffer

			//get length of header and content length (snprintf doesnt write, formats and returns size of string)
			size_t length_of_content_length = snprintf(NULL, 0, contentlen, length_of_response);
			size_t length_of_header = strlen(header) + length_of_content_length;

			//move data to fit header
			memmove(&responsebuffer[length_of_header], responsebuffer, length_of_response);

			size_t amount_written = sprintf(responsebuffer, "%s", header); //write header

			//actually write the content length, sprintf writes null terminator so we have to overwrite it
			char first_response_char = responsebuffer[amount_written + length_of_content_length]; //+1 for null term, length_of_content_length points to char after end of string (null)
			sprintf(&responsebuffer[amount_written], contentlen, length_of_response);
			responsebuffer[amount_written + length_of_content_length] = first_response_char;
		}

		//cleanup
		for(size_t i = 0; i < WEBSERVER_AMOUNT_OF_RESULTS; i++) {
			webserver_free_page_info(&bestresults[i]);
			resultscores[i] = 0.0;
		}
		memset(bestresults, 0, sizeof(webserver_page_info)*WEBSERVER_AMOUNT_OF_RESULTS);

		free(query);
		pageinfobestscore = 0.0;
		pageinfoworstscore = 0.0;

		//send data and close socket
		webserver_socketlib_send(rs, responsebuffer, strlen(responsebuffer));
		webserver_socketlib_close(rs);

		fprintf(stdout, "done!\n");
	}

	//cleanup

	fprintf(stdout, "Cleaning up...\n");

	webserver_wid_close(&wid);
	webserver_socketlib_close(s);
	webserver_socketlib_close(rs);
	webserver_socketlib_terminate();
}
