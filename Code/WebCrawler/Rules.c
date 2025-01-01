#include "Rules.h"

#define WEBCRAWL_MAX_SITEMAP_LEVELS 5

//false return value indicated that sitemap is too deep
bool internal_get_sitemap(CURL* aphandle, unsigned char* araddress, webcrawl_queue* apqueue, size_t asitemaplevel) {
	if(asitemaplevel >= WEBCRAWL_MAX_SITEMAP_LEVELS) {
		return false;
	}

	webcrawl_http_response_data internal_buffer;
	webcrawl_init_response_buffer(&internal_buffer, araddress);

	webcrawl_request_outcomes result = webcrawl_make_request(&internal_buffer, aphandle);
	switch(result) {
		case(WEBCRAWL_REQ_OK):
			break;
		case(WEBCRAWL_REQ_NOT_FOUND):
		case(WEBCRAWL_REQ_DNS_ERROR):
		case(WEBCRAWL_REQ_ERROR):
			fprintf(stdout, "Sitemap load error %i \n", result);
			webcrawl_free_response_data(&internal_buffer);
			return true;
	}

	unsigned char* data = (unsigned char*)curl_easy_unescape(NULL, (char*)internal_buffer.data, 0, NULL);

	webcrawl_free_response_data(&internal_buffer);

	//50MiB max
	if(strlen((char*)data) >= 52428800) {
		fprintf(stdout, "Sitemap file is too big (>50MB)\n");
		free(data);
		return true;
	}

	htmlDocPtr document = xmlReadDoc(data, NULL, NULL, XML_PARSE_NOBLANKS | XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET);
	if(!document) {
		fprintf(stdout, "libxml2 Sitemap Parsing failed. %s\n", xmlGetLastError()->message);
		free(data);
		xmlFreeDoc(document);
		return true;
	}

	free(data);

	xmlNode* root;
	root = xmlDocGetRootElement(document);
	uint8_t sitemap_type = UINT8_MAX; //0 is for URL sitemap, 1 is for sitemap of sitemaps

	if(strcmp((char*)root->name, "urlset") == 0) sitemap_type = 0;
	else if(strcmp((char*)root->name, "sitemapindex") == 0) sitemap_type = 1;
	else {
		fprintf(stdout, "Invalid sitemap, skipping.\n");
		xmlFreeDoc(document);
		return true;
	}

	//structure of sitemap (according to standard):
	//<urlset> <sitemapindex>
	//	<url>     <sitemap>
	//	</url>    </sitemap>
	//</urlset> </sitemapindex>
	//https://sitemaps.org/protocol.html
	for(xmlNode* current_node = root->children; current_node; current_node = current_node->next) {
		if(
			(sitemap_type == 0 && strcmp((char*)current_node->name, "url") == 0) ||
			(sitemap_type == 1 && strcmp((char*)current_node->name, "sitemap") == 0)
		) {
			for(xmlNode* child_node = current_node->children; child_node; child_node = child_node->next) {
				if(strcmp((char*)child_node->name, "loc") == 0) {
					xmlChar* url = xmlNodeGetContent(child_node);
					if(url) {
						if(sitemap_type == 0) {
							//save URL to queue
							webcrawl_append_to_queue(apqueue, url, NULL);
						}
						else if(sitemap_type == 1) {
							//crawl another sitemap (recursively) - limit of WEBCRAWL_MAX_SITEMAP_LEVELS sitemaps deep
							if(!internal_get_sitemap(aphandle, url, apqueue, asitemaplevel++)) {
								xmlFreeDoc(document);
								xmlFree(url);
								return false; //too deep, return
							}
						}
						xmlFree(url);
					}
					break;
				}
			}
		}
		else {
			fprintf(stdout, "Invalid sitemap, some data saved.\n");
			xmlFreeDoc(document);
			return true;
		}
	}

	xmlFreeDoc(document);
	return true;
}

const unsigned char* crobotstxt = (unsigned char*)"/robots.txt";
void webcrawl_get_domain_info(CURL* aphandle, unsigned char* ardomain, webcrawl_drd_entry* apdrdentry, webcrawl_queue* apqueue) {
	//add basic data to drd entry
	size_t length_of_domain = strlen((const char*)ardomain);
	apdrdentry->domainname = malloc(length_of_domain+1);
	memcpy(apdrdentry->domainname, ardomain, length_of_domain);
	apdrdentry->domainname[length_of_domain] = '\0';
	apdrdentry->last_crawled = time(NULL);
	//domainid assigned automatically by DRD

	//robots.txt - length 11 + null terminator
	unsigned char* robots_address = malloc(length_of_domain+12);
	memcpy(robots_address, ardomain, length_of_domain);
	memcpy(&robots_address[length_of_domain], crobotstxt, 12); //includes null terminator

	//init internal response buffer
	webcrawl_http_response_data internal_buffer;
	webcrawl_init_response_buffer(&internal_buffer, robots_address);
	free(robots_address);
	webcrawl_request_outcomes result = webcrawl_make_request(&internal_buffer, aphandle);
	switch(result) {
		case(WEBCRAWL_REQ_OK):
			if(internal_buffer.current_size == 0) {
				fprintf(stdout, "Robots.txt error: file is empty!\n");
				webcrawl_free_response_data(&internal_buffer);
				return;
			}
			break;
		default:
			fprintf(stdout, "Robots.txt load error %i \n", result);
			webcrawl_free_response_data(&internal_buffer);
			return;
	}

	size_t response_size = strlen((char*)internal_buffer.data);
	//according to standard, limit should be at least 500 KiB = 512 KB
	if(response_size > 512000) {
		webcrawl_free_response_data(&internal_buffer);
		fprintf(stdout, "Robots.txt file is too big (%zu bytes) for domain %s\n", response_size, ardomain);
		return;
	}

	//decode data
	unsigned char* data = (unsigned char*)curl_easy_unescape(NULL, (char*)internal_buffer.data, response_size, NULL);

	//free data not needed after request made and unescaped
	webcrawl_free_response_data(&internal_buffer);

	//split into lines
	unsigned char** lines;
	size_t lines_amount = 0;
	webcrawl_split_ustr(data, '\n', &lines, &lines_amount, 0);

	//free original data
	curl_free(data);

	//flags for parsing
	bool isrelated = false; //set this flag when reading data for *
	bool isourbot = false; //set this flag when reading data for our bot
	bool wasourbot = false; //set this flag if we read data for out bot
	bool lastuseragent = false; //set this flag when there are multiple lines of the user-agent directive

	//$ ends match
	//* anything between
	//https://www.rfc-editor.org/rfc/rfc9309.html#name-special-characters

	unsigned char** sections = NULL;
	size_t sections_amount = 0;

	//minimize memory allocation calls, massive bottleneck
	size_t allocated_for_allowed = 10;
	size_t allocated_for_disallowed = 10;

	apdrdentry->allowed = malloc(allocated_for_allowed*sizeof(unsigned char*));
	apdrdentry->disallowed = malloc(allocated_for_disallowed*sizeof(unsigned char*));

	for(size_t n = 0; n < lines_amount; n++) {
		if(lines[n][0] == '#') continue; //skip comments
		if(strlen((char*)lines[n]) == 0) continue; //skip empty lines

		webcrawl_free_ustr_array(sections, sections_amount); //free here so we avoid double free at the end (at beginning value is still NULL)
		char* comment = strchr((char*)lines[n], '#');
		if(comment) {
			lines[n][comment - (char*)lines[n]] = '\0';
		}

		webcrawl_remove_space_ustr(&lines[n]); //remove whitespace

		sections = NULL;
		sections_amount = 0;
		webcrawl_split_ustr(lines[n], ':', &sections, &sections_amount, 2); // split by colon, as per standard (Name: Value) - split by first delimiter (maximum 2 splits)
		if(sections_amount == 0) {
			//something wrong

			for(size_t i = 0; i < apdrdentry->amount_disallowed; i++) {
				free(apdrdentry->disallowed[i]);
			}
			apdrdentry->amount_disallowed = 0;

			for(size_t i = 0; i < apdrdentry->amount_allowed; i++) {
				free(apdrdentry->allowed[i]);
			}
			apdrdentry->amount_allowed = 0;

			//rest freed at end of function

			break;
		};
		unsigned char* dataloopover = sections[0]; //functions ALWAYS returns at least 1 element
		for(;*dataloopover;dataloopover++) *dataloopover = tolower(*dataloopover); //case insensitive sections

		//ignore if user agent anything else than our bot or *
		if(strcmp((char*)sections[0], "user-agent") == 0) {
			if(isourbot && !lastuseragent) {
				//we already got info about our bot - ignore input until our section again (scan to find sitemap)
				isrelated = false;
				isourbot = false;
			}
			if(sections_amount == 1) {
				//invalid robots.txt
				fprintf(stdout, "Malformed robots.txt, following rules before malformed line!\n");
				break;
			}
			if(strcmp((char*)sections[1], WEBCRAWL_USER_AGENT_SHORT) == 0) {
				//our bot
				if(isrelated) {
					//we got info for all bots, but now it is for us specifically - clear collected data
					for(size_t i = 0; i < apdrdentry->amount_disallowed; i++) {
						free(apdrdentry->disallowed[i]);
					}
					free(apdrdentry->disallowed);
					apdrdentry->disallowed = NULL;
					apdrdentry->amount_disallowed = 0;

					for(size_t i = 0; i < apdrdentry->amount_allowed; i++) {
						free(apdrdentry->allowed[i]);
					}
					free(apdrdentry->allowed);
					apdrdentry->allowed = NULL;
					apdrdentry->amount_allowed = 0;
				}
				isrelated = false;
				isourbot = true;
				wasourbot = true;
			}
			else if(!wasourbot && strcmp((char*)sections[1], "*") == 0) {
				//all bots
				isrelated = true;
			}
			else if(!lastuseragent) {
				//none of the above
				isrelated = false;
				isourbot = false;
			}
			else {
				//but our declaration is part of multiple user-agent directives so do nothing
			}

			lastuseragent = true;
		}
		//sitemap affects everyone
		else if(strcmp((char*)sections[0], "sitemap") == 0) {
			if(sections_amount == 1) {
				//invalid robots.txt
				fprintf(stdout, "Malformed robots.txt, following rules before malformed line!\n");
				break;
			}

			size_t sitemap_future_length = strlen((char*)sections[1]);
			unsigned char* sitemap_address = malloc(sitemap_future_length+1);
			memcpy(sitemap_address, sections[1], sitemap_future_length);
			sitemap_address[sitemap_future_length] = '\0';

			internal_get_sitemap(aphandle, sitemap_address, apqueue, 0);

			free(sitemap_address);

			lastuseragent = false;
		}
		//allow and disallow - only if section affects us (our or * user agent)
		else if((isrelated || isourbot) && strcmp((char*)sections[0], "allow") == 0) {
			if(apdrdentry->amount_allowed == allocated_for_allowed) {
				allocated_for_allowed = allocated_for_allowed*2;
				apdrdentry->allowed = realloc(apdrdentry->allowed, allocated_for_allowed*sizeof(unsigned char*));
			}
			apdrdentry->amount_allowed++;
			if(sections_amount == 1) {
				//nothing is allowed, save anyway since there might be an disallow directive later
				apdrdentry->allowed[apdrdentry->amount_allowed-1] = malloc(1);
				apdrdentry->allowed[apdrdentry->amount_allowed-1][0] = '\0';
			}
			else {
				size_t length_of_value = strlen((char*)sections[1]);
				apdrdentry->allowed[apdrdentry->amount_allowed-1] = malloc(length_of_value+1);
				memcpy(apdrdentry->allowed[apdrdentry->amount_allowed-1], sections[1], length_of_value);
				apdrdentry->allowed[apdrdentry->amount_allowed-1][length_of_value] = '\0';
			}

			lastuseragent = false;
		}
		else if((isrelated || isourbot) && strcmp((char*)sections[0], "disallow") == 0) {
			if(apdrdentry->amount_disallowed == allocated_for_disallowed) {
				allocated_for_disallowed = allocated_for_disallowed*2;
				apdrdentry->disallowed = realloc(apdrdentry->disallowed, allocated_for_disallowed*sizeof(unsigned char*));
			}
			apdrdentry->amount_disallowed++;
			if(sections_amount == 1) {
				//nothing is disallowed, save anyway since there might be an allow directive later
				apdrdentry->disallowed[apdrdentry->amount_disallowed-1] = malloc(1);
				apdrdentry->disallowed[apdrdentry->amount_disallowed-1][0] = '\0';
			}
			else {
				size_t length_of_value = strlen((char*)sections[1]);
				apdrdentry->disallowed[apdrdentry->amount_disallowed-1] = malloc(length_of_value+1);
				memcpy(apdrdentry->disallowed[apdrdentry->amount_disallowed-1], sections[1], length_of_value);
				apdrdentry->disallowed[apdrdentry->amount_disallowed-1][length_of_value] = '\0';
			}

			lastuseragent = false;
		}
		//crawl delay - only if section affects us (our or * user agent)
		else if((isrelated || isourbot) && strcmp((char*)sections[0], "crawl-delay") == 0) {
			if(sections_amount == 1) {
				//invalid robots.txt
				fprintf(stdout, "Malformed robots.txt, following rules before malformed line!\n");
				break;
			}
			apdrdentry->seconds_between_requests = atoll((char*)sections[1]);

			lastuseragent = false;
		}
		//something else: might be extension not supported by NPWS, just skip and dont bother
		else {
			lastuseragent = false;
		}
	}

	//shrink memory to needed size

	if(apdrdentry->amount_allowed == 0) { free(apdrdentry->allowed); apdrdentry->allowed = NULL; }
	else apdrdentry->allowed = realloc(apdrdentry->allowed, apdrdentry->amount_allowed*sizeof(unsigned char*));

	if(apdrdentry->amount_disallowed == 0) { free(apdrdentry->disallowed); apdrdentry->disallowed = NULL; }
	else apdrdentry->disallowed = realloc(apdrdentry->disallowed, apdrdentry->amount_disallowed*sizeof(unsigned char*));

	//free arrays
	webcrawl_free_ustr_array(sections, sections_amount);
	webcrawl_free_ustr_array(lines, lines_amount);
}

//robots.txt check

bool internal_matches_pattern(unsigned char* araddress, unsigned char** appdirectives, size_t adirectiveamount) {
	for(size_t n = 0; n < adirectiveamount; n++) {
		if(!appdirectives[n]) continue;

		unsigned char* clean = webcrawl_clean_address(araddress);
		unsigned char** split_array = NULL;
		size_t split_array_size = 0;
		//split directives by "any character" place, compare each section to string in order
		webcrawl_split_ustr(appdirectives[n], '*', &split_array, &split_array_size, 0);
		if(split_array_size == 0) {
			free(clean);
			//always must be above one, empty string just gets ignored
			continue;
		}

		size_t offset = 0;
		size_t amount_matches = 0;
		for(size_t i = 0; i < split_array_size; i++) {
			char* ptr;
			char* dollarptr = strchr((char*)split_array[i], '$'); //check if contains end of match character
			if(dollarptr) {
				//we found $ - end of match character

				*dollarptr = '\0'; //end string so we do not compare the $
				ptr = strstr((char*)&clean[offset], (char*)split_array[i])+strlen((char*)split_array[i]); //find substr

				//calculate condition before freeing data
				//condition check if any string remaining after $ character (so if exact match or not)
				//stuff before is equal since if it isnt we break out of the loop and never reach this
				bool condition;

				if(ptr - (char*)clean > strlen((char*)clean)) condition = false; //if less characters also not equal (prevent segfault)
				else condition = *(ptr) == '\0';

				free(clean);
				webcrawl_free_ustr_array(split_array, split_array_size);

				return condition; //match equal (whole string same) or match false (not completely equal (has some chars later))
			}

			//normal entry
			ptr = strstr((char*)&clean[offset], (char*)split_array[i]);
			if(ptr) {
				amount_matches++;
				offset = (ptr - (char*)clean);
			}
			else break;
		}

		//if dollar sign present and offset != size of string, match not complete

		free(clean);
		webcrawl_free_ustr_array(split_array, split_array_size);

		if(amount_matches == split_array_size) {
			return true;
		}
	}

	return false;
}

bool webcrawl_check_if_allowed(unsigned char* araddress, webcrawl_drd_entry* apdrdentry) {
	//robots.txt special characters
	//$ - exactly match with string (end of match pattern)
	//* - any characters are allowed there (check only parts between them)

	//get only the URL part of address

	unsigned char* localpath = webcrawl_get_path(araddress);

	//match with allowed
	if(internal_matches_pattern(araddress, apdrdentry->allowed, apdrdentry->amount_allowed)) {
		free(localpath);
		return true;
	}

	//matched with disallowed
	if(internal_matches_pattern(araddress, apdrdentry->disallowed, apdrdentry->amount_disallowed)) {
		free(localpath);
		return false;
	}

	free(localpath);

	//no directive: implicitly allowed
	return true;
}
