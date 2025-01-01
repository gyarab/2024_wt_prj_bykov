#ifndef WEBCRAWL_FILE_TYPES
#define WEBCRAWL_FILE_TYPES
#include "Utils.h"

// Queue management

typedef struct {
	unsigned char* url;

	//pointers point to same type or NULL if at beginning / end
	void* prev;
	void* next;
} webcrawl_queue_node;

void webcrawl_init_queue_node(webcrawl_queue_node* apnode);
//frees a node ALREADY DETACHED FROM QUEUE
void webcrawl_free_queue_node(webcrawl_queue_node* apnode);

typedef struct {
	webcrawl_queue_node* first;
	webcrawl_queue_node* last;
} webcrawl_queue;

void webcrawl_init_queue(webcrawl_queue* apqueue);
void webcrawl_free_queue(webcrawl_queue* apqueue);

//if apfirstnode is NULL then it becomes the first node
void webcrawl_append_to_queue(webcrawl_queue* apqueue, unsigned char* arfutureaddr, unsigned char* arcurrentaddr);
//pointer to node is stored in apnode (free it later!) and is removed from queue
webcrawl_queue_node* webcrawl_get_first_from_queue(webcrawl_queue* apqueue);
//reads domains (separated by newline) to queue from a file
void webcrawl_append_to_queue_from_file(webcrawl_queue* apqueue, const char* arfilename);

void webcrawl_print_queue_node(webcrawl_queue_node* apnode);
void webcrawl_print_queue(webcrawl_queue* apqueue);

// Response buffer management

typedef struct {
	unsigned char* data; //data of response
	size_t current_size; //size of response
	unsigned char* url;
	size_t allocated_size; //size currently allocated for response, optimization for less memory allocations
} webcrawl_http_response_data;

void webcrawl_init_response_buffer(webcrawl_http_response_data* apdata, unsigned char* arurl);
void webcrawl_null_terminate_response(webcrawl_http_response_data* apdata);

size_t webcrawl_http_get_callback(char* apdata, size_t asize, size_t aamountbytes, void* apresponse_info);

typedef enum {
	WEBCRAWL_REQ_ERROR = 0,
	WEBCRAWL_REQ_OK = 1,
	WEBCRAWL_REQ_NOT_FOUND = 2,
	WEBCRAWL_REQ_DNS_ERROR = 3
} webcrawl_request_outcomes;

webcrawl_request_outcomes webcrawl_make_request(webcrawl_http_response_data* apdata, CURL* aphandle);
void webcrawl_free_response_data(webcrawl_http_response_data* apdata);

// Page info management

typedef struct {
	//ID in database - every page has own (e.g. every wikipedia article)
	size_t id;
	//Domain ID in database - used to find or load robots.txt file and domain name from DRD
	size_t domain_id;
	//Address
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
} webcrawl_page_info;

void webcrawl_init_page_info(webcrawl_page_info* const appageinfo);
void webcrawl_free_page_info(webcrawl_page_info* const appageinfo);

void webcrawl_print_page_info(webcrawl_page_info* const appageinfo);

#endif
