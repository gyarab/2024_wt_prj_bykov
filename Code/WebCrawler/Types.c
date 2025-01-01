#include "Types.h"

//queue

void webcrawl_init_queue_node(webcrawl_queue_node* apnode) {
	if(!apnode) { return; }
	apnode->url = NULL;
	apnode->prev = NULL;
	apnode->next = NULL;
}
void webcrawl_free_queue_node(webcrawl_queue_node* apnode) {
	if(!apnode) { return; } //NULL check
	if(apnode->prev != NULL || apnode->next != NULL) { return; } //still in queue
	free(apnode->url);
	free(apnode);
}

void webcrawl_init_queue(webcrawl_queue* apqueue) {
	apqueue->first = NULL;
	apqueue->last = NULL;
}
void webcrawl_free_queue(webcrawl_queue* apqueue) {
	if(!apqueue->first) { return; } //queue is empty

	webcrawl_queue_node* node = apqueue->first;
	while(true) {
		free(node->prev); //clears node before, if NULL (when processing node 1) does nothing
		free(node->url); //clears node url
		if(!node->next) {
			free(node); break; //free last remaining node and break out
		}
		node = node->next;
	}
	apqueue->first = NULL;
	apqueue->last = NULL;
}

void webcrawl_append_to_queue(webcrawl_queue* apqueue, unsigned char* arfutureaddr, unsigned char* arcurrentaddr) {
	webcrawl_queue_node* new_element = malloc(sizeof(webcrawl_queue_node));

	//if only one of the 2 params provided, fill from the second one
	if(!arfutureaddr) {
		//if no future address, assume current
		size_t length_of_current = strlen((char*)arcurrentaddr);
		if(length_of_current < 4) {
			//minimum address is a.bb, 4 characters - do not throw error, will just fail when getting request (just performance)
			return;
		}
		new_element->url = webcrawl_clean_address(arcurrentaddr);
	}
	else if(!arcurrentaddr) {
		//if no current address, assume future
		size_t length_of_future = strlen((char*)arfutureaddr);
		if(length_of_future < 4) {
			return;
		}
		new_element->url = webcrawl_clean_address(arfutureaddr);
	}
	else {
		//otherwise join them
		new_element->url = webcrawl_join_address(arfutureaddr, arcurrentaddr);
		//if something wrong
		if(!new_element->url) {
			//no need to free url, is NULL
			free(new_element);
			return;
		}
	}

	//queue pointers
	new_element->next = NULL;
	new_element->prev = apqueue->last;

	if(!apqueue->first) {
		//set element as first in empty queue
		apqueue->first = new_element;
		apqueue->last = new_element;
	}
	else {
		//append element to existing queue
		apqueue->last->next = new_element;
		apqueue->last = new_element;
	}
}
webcrawl_queue_node* webcrawl_get_first_from_queue(webcrawl_queue* apqueue) {
	if(!apqueue->first) return NULL; //empty queue!

	webcrawl_queue_node* node = apqueue->first; //get the current first node
	apqueue->first = apqueue->first->next; //set new first node

	if(apqueue->first == NULL) {
		apqueue->first = NULL; //queue is now empty
		apqueue->last = NULL;
	}
	else { //if anything remains
		apqueue->first->prev = NULL; //is first
	}

	node->next = NULL; //set that first node has no next, SET AFTER WE USE IT TO FIND NEXT
	return node;
}

void webcrawl_append_to_queue_from_file(webcrawl_queue* apqueue, const char* arfilename) {
	FILE* handle = fopen(arfilename, "r");
	if(!handle) {
		fprintf(stdout, "Unable to open domain seed file %s - check if the file exists and try again.\n", arfilename);
		exit(EXIT_FAILURE);
	}

	char line[2048]; //1024 characters should be enough
	while(!feof(handle)) {
		if(!fgets(line, 2048, handle)) {
			break;
		}
		line[strlen(line)-1] = '\0'; //remove last newline
		webcrawl_append_to_queue(apqueue, (unsigned char*)line, NULL);
	}

	fclose(handle);
}

void webcrawl_print_queue_node(webcrawl_queue_node* apnode) {
	if(!apnode) { return; }

	fprintf(stdout, "URL: %s, prev: %p, next: %p \n", apnode->url, apnode->prev, apnode->next);
}
void webcrawl_print_queue(webcrawl_queue* apqueue) {
	if(!apqueue->first) { return; }

	webcrawl_queue_node* node = apqueue->first;
	while(true) {
		webcrawl_print_queue_node(node);
		if(!node->next) break;
		node = node->next;
	}
}

// http_response_data management

void webcrawl_init_response_buffer(webcrawl_http_response_data* apdata, unsigned char* arurl) {
	apdata->data = NULL;
	apdata->current_size = 0;
	apdata->allocated_size = 0;

	size_t url_length = strlen((const char*)arurl);
	apdata->url = malloc((url_length+1));
	memcpy(apdata->url, arurl, url_length);
	apdata->url[url_length] = '\0';
}
void webcrawl_null_terminate_response(webcrawl_http_response_data* apdata) {
	apdata->data = realloc(apdata->data, apdata->current_size+1);
	apdata->data[apdata->current_size] = '\0';
}

//callback for CURL
size_t webcrawl_http_get_callback(char* apdata, size_t asize, size_t aamountbytes, void* apresponse_info) {
	webcrawl_http_response_data* saver = ((webcrawl_http_response_data*)apresponse_info);
	if(saver->current_size+aamountbytes >= saver->allocated_size) {
		saver->allocated_size += aamountbytes; //guarantee that section will be read fully
		saver->allocated_size *= 2;
		saver->data = realloc(saver->data, saver->allocated_size);
	}
	memcpy(&saver->data[saver->current_size], (unsigned char*)apdata, aamountbytes);
	saver->current_size += aamountbytes;
	return aamountbytes;
}

//3MB, slightly more than average for websites (avoid reallocing at all costs, massive bottleneck)
#define WEBCRAWL_PREALLOCATED_REQUEST_DATA_BUFFER_SIZE 3000000

webcrawl_request_outcomes webcrawl_make_request(webcrawl_http_response_data* apdata, CURL* aphandle) {
	curl_easy_setopt(aphandle, CURLOPT_URL, apdata->url);
	curl_easy_setopt(aphandle, CURLOPT_WRITEDATA, apdata);

	#ifdef WEBCRAWL_DEBUG
	fprintf(stdout, "Making request to %s...", apdata->url);
	#endif
	apdata->allocated_size = WEBCRAWL_PREALLOCATED_REQUEST_DATA_BUFFER_SIZE;
	apdata->data = malloc(apdata->allocated_size);
	memset(apdata->data, 0, WEBCRAWL_PREALLOCATED_REQUEST_DATA_BUFFER_SIZE);
	apdata->current_size = 0;
	CURLcode result = curl_easy_perform(aphandle);
	long request_code = 0;
	if(result != CURLE_OK) {
		free(apdata->data);
		apdata->data = NULL;
		apdata->allocated_size = 0;

		curl_easy_getinfo(aphandle, CURLINFO_RESPONSE_CODE, &request_code);
		if(result == CURLE_COULDNT_RESOLVE_HOST || result == CURLE_COULDNT_RESOLVE_PROXY) {
			fprintf(stdout, "Request failed: DNS failure, URL: %s \n", apdata->url);
			return WEBCRAWL_REQ_DNS_ERROR;
		}
		if(request_code == 404) {
			fprintf(stdout, "Request failed: 404 Not Found, URL: %s \n", apdata->url);
			return WEBCRAWL_REQ_NOT_FOUND;
		}
		fprintf(stdout, "Request failed: %s, URL: %s \n", curl_easy_strerror(result), apdata->url);
		//no exit - just return error
		return WEBCRAWL_REQ_ERROR;
	}

	if(apdata->current_size == 0) free(apdata->data);
	else {
		//shrink data to needed size
		apdata->data = realloc(apdata->data, apdata->current_size);
		//only null terminate on success (when not freed)
		webcrawl_null_terminate_response(apdata);
	}
	apdata->allocated_size = apdata->current_size;

	#ifdef WEBCRAWL_DEBUG
	fprintf(stdout, "200 OK.\n");
	#endif

	return WEBCRAWL_REQ_OK;
}
void webcrawl_free_response_data(webcrawl_http_response_data* apdata) {
	if(apdata->allocated_size != 0) {
		free(apdata->data);
		apdata->current_size = 0;
		apdata->allocated_size = 0;
	}

	free(apdata->url);
	apdata = NULL;
}

// page_info management

void webcrawl_init_page_info(webcrawl_page_info* const appageinfo) {
	appageinfo->id = SIZE_MAX;
	appageinfo->domain_id = SIZE_MAX;
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
void webcrawl_free_page_info(webcrawl_page_info* const appageinfo) {
	appageinfo->id = 0;
	appageinfo->domain_id = 0;

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

void webcrawl_print_page_info(webcrawl_page_info* const appageinfo) {
	webcrawl_time_output time_formatted = webcrawl_get_correct_time_unit((size_t)time(NULL)-appageinfo->last_visited);
	fprintf(
		stdout,
		"Page %zu/D%zu ; title %s ; header %s ; author %s ; %zu keywords ; %zu description words; %zu from %zu to ; %zu people visited ; crawler %zu%c ago\n",
		appageinfo->id, appageinfo->domain_id, appageinfo->title, appageinfo->header1, appageinfo->author, appageinfo->keywords_amount, appageinfo->description_words_amount,
		appageinfo->amount_links_from, appageinfo->amount_links_to, appageinfo->amount_visitors, time_formatted.value, time_formatted.unit
	);
}
