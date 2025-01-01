#include "Noreturn.h"

//TODO (for later, between march and may 18?) - MAYBE add multithreading?
//TODO measure program running time, calculate pages per second (more like seconds per page) at end of program
//TODO recieve permission from LinkedIn whitelist-crawl@linkedin.com and instagram

int main(int aargumentamount, char** aprarguments) {
	assert((curl_version_info(CURL_VERSION_THREADSAFE)->features & CURL_VERSION_THREADSAFE) != 0);
	assert(sqlite3_threadsafe() == 1);
	assert(sizeof(uint64_t) == 8); //important for future calculations
	signal(SIGINT, webcrawl_keyboard_interrupt_handler); //on keyboard exit

	if(aargumentamount != 2) webcrawl_print_error_menu();
	if(strcmp(aprarguments[1], "--help") == 0 || strcmp(aprarguments[1], "-h") == 0) webcrawl_print_help_menu();

	fprintf(stdout, "NPWS WebCrawler\n(c) Martin Bykov 2024-2025\n");

	// init CURL

	CURL* handle = curl_easy_init();
	if(!handle) {
		fprintf(stdout, "CURL failed to init.\n");
		exit(EXIT_FAILURE);
	}

	// setup library parameters

	LIBXML_TEST_VERSION;

	sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	sqlite3_config(SQLITE_CONFIG_MMAP_SIZE, INT32_MAX-100000); //4GB of RAM

	//things that dont change between requests
	curl_easy_setopt(handle, CURLOPT_USERAGENT, WEBCRAWL_USER_AGENT);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, webcrawl_http_get_callback);
	curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10);
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1); //for multithreading

	// actual crawling
	// initialize variables

	//variables
	webcrawl_page_info pageinfo;
	webcrawl_init_page_info(&pageinfo);
	webcrawl_drd_entry domain;
	webcrawl_init_drd_entry(&domain);
	webcrawl_queue address_queue;
	webcrawl_init_queue(&address_queue);
	webcrawl_append_to_queue_from_file(&address_queue, aprarguments[1]); //add first URL into queue

	//databases
	webcrawl_wid_database wid;
	webcrawl_wid_open(&wid);
	webcrawl_drd_database drd;
	webcrawl_drd_open(&drd);

	webcrawl_set_keyboard_interrupt_handler(&wid, &drd, &address_queue); //for safe exit of databases, visit Noreturn.c

	// main crawling loop

	webcrawl_http_response_data buffer;
	webcrawl_queue_node* qn = NULL;
	while(true) {
		webcrawl_free_queue_node(qn);
		webcrawl_free_drd_entry(&domain);

		qn = webcrawl_get_first_from_queue(&address_queue);
		if(!qn) {
			fprintf(stdout, "No more addresses in queue!\n");
			break;
		};

		//check if is in DRD
		webcrawl_init_drd_entry(&domain);
		unsigned char* cleaned_domain = webcrawl_get_domain(qn->url); //have to use separate variable, cannot assign to domain.domainname because overriden by function
		domain.pages_amount++;

		if(webcrawl_drd_get_entry_by_name(&drd, cleaned_domain, &domain) == 0) {
			fprintf(stdout, "New domain (%s): Visiting robots and sitemap!\n", cleaned_domain);

			//load robots.txt, sitemap and add it to DB
			webcrawl_get_domain_info(handle, cleaned_domain, &domain, &address_queue);
			webcrawl_drd_add_entry(&drd, &domain);
		}
		//update data later, we check for crawl delay

		free(cleaned_domain);

		//check if we can visit the page
		if(!webcrawl_check_if_allowed(qn->url, &domain)) {
			//not allowed - skip
			fprintf(stdout, "Page not allowed by robots.txt: skipping page %s\n", qn->url);
			continue;
		}

		//check for crawl delay
		if(time(NULL) - domain.last_crawled < domain.seconds_between_requests) {
			fprintf(stdout, "Waiting for domain crawl-delay, skipping page! (%zus/%zus)\n", time(NULL) - domain.last_crawled, domain.seconds_between_requests);
			webcrawl_append_to_queue(&address_queue, qn->url, NULL); //push to back of queue
			continue;
		}

		//update relevant data in DRD AFTER CHECKING CRAWL DELAY!
		domain.last_crawled = time(NULL);
		webcrawl_drd_update_entry(&drd, &domain);

		webcrawl_init_page_info(&pageinfo);

		//check if is in WID
		if(webcrawl_wid_get_entry_by_address(&wid, qn->url, &pageinfo) == 1) {
			//we visited this at some point

			if(time(NULL)-pageinfo.last_visited > WEBCRAWL_REVISIT_TIME) {
				//time to recrawl - remove WID element, will get added later
				webcrawl_wid_delete_element(&wid, &pageinfo);
				webcrawl_free_page_info(&pageinfo);
			}
			else {
				//not yet time to recrawl
				fprintf(stdout, "Skipping page %s\n", pageinfo.url);
				pageinfo.amount_links_to++;
				pageinfo.last_visited = time(NULL);
				webcrawl_wid_update_entry(&wid, &pageinfo);
				webcrawl_free_page_info(&pageinfo);
				continue;
			}
		}

		webcrawl_init_response_buffer(&buffer, qn->url);
		webcrawl_request_outcomes req_result = webcrawl_make_request(&buffer, handle);
		switch(req_result) {
			case(WEBCRAWL_REQ_OK):
				//if ok: process the page
				webcrawl_free_page_info(&pageinfo); //free here so we reset it
				webcrawl_init_page_info(&pageinfo);
				webcrawl_process_page_result result = webcrawl_process_page(&buffer, &pageinfo, &address_queue);
				if(result & WEBCRAWL_PAGE_RESTRICT_NOINDEX) {
					fprintf(stdout, "Page not indexed: meta robots noindex set.\n");
				}
				else {
					pageinfo.domain_id = webcrawl_drd_get_domain_id_by_name(&drd, domain.domainname);
					webcrawl_wid_add_entry(&wid, &pageinfo);
					webcrawl_drd_update_entry(&drd, &domain);
				}
				if(result & WEBCRAWL_PAGE_RESTRICT_NOFOLLOW) {
					//nofollow directive is handled in process page
					fprintf(stdout, "Page not followed: meta robots nofollow set.\n");
				}
				webcrawl_free_page_info(&pageinfo);
				break;
			case(WEBCRAWL_REQ_NOT_FOUND):
			case(WEBCRAWL_REQ_DNS_ERROR):
			case(WEBCRAWL_REQ_ERROR):
				fprintf(stdout, "Invalid page, skipping!\n");
				break;
		}

		webcrawl_free_response_data(&buffer);
	}

	fprintf(stdout, "Done!\n");

	fprintf(stdout, "Cleaning queue\n");
	webcrawl_free_queue(&address_queue);

	fprintf(stdout, "Closing DBs\n");
	webcrawl_wid_close(&wid);
	webcrawl_drd_close(&drd);

	fprintf(stdout, "Cleaning libs\n");
	curl_easy_cleanup(handle);
	xmlCleanupParser();

	exit(EXIT_SUCCESS);
}
