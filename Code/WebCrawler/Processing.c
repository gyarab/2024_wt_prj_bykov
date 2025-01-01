#include "Processing.h"

// processing page

typedef struct {
	bool has_title; //only use the first title tag if page has multiple
	bool has_header; //only use the first header tag if page has multiple
	bool has_author; //only use the first author if page has multiple
	webcrawl_process_page_result result; //if nofollow set ignore addresses
} internal_page_data_status;

//returns true if everything ok, false if
webcrawl_process_page_result internal_evaluate_node(internal_page_data_status* apstatus, xmlNode* apnode, webcrawl_page_info* const appageinfo, webcrawl_queue* apqueue, const webcrawl_http_response_data* const apdata) {
	if(apnode->type != XML_ELEMENT_NODE) return WEBCRAWL_PAGE_RESTRICT_OK;

	if((!(apstatus->result & WEBCRAWL_PAGE_RESTRICT_NOFOLLOW)) && (strcmp((char*)apnode->name, "a") == 0)) {
		unsigned char* href = xmlGetProp(apnode, (xmlChar*)"href");
		if(!href) {
			fprintf(stdout, "Invalid page address tag content!\n");
			return WEBCRAWL_PAGE_RESTRICT_OK;
		}
		appageinfo->amount_links_from++;
		webcrawl_append_to_queue(apqueue, href, apdata->url);
		xmlFree(href);
	}
	else if(strcmp((char*)apnode->name, "meta") == 0) {
		unsigned char* name = xmlGetProp(apnode, (xmlChar*)"name");
		unsigned char* content = xmlGetProp(apnode, (xmlChar*)"content");
		if(name && content) {
			if(strcmp((char*)name, "robots") == 0 && strlen((char*)content) >= 7) {
				//duplicate string due to different free functions (free/xmlFree)
				unsigned char* string_duplicate = (unsigned char*)strdup((char*)content);
				webcrawl_remove_space_ustr(&string_duplicate);

				//split array by comma
				unsigned char** string_dup_array = NULL;
				size_t string_dup_size = 0;
				webcrawl_split_ustr(string_duplicate, ',', &string_dup_array, &string_dup_size, 0);

				//free not needed data
				free(string_duplicate);

				if(!string_dup_array) {
					//wrong string, ignore
					xmlFree(name); xmlFree(content);
					return WEBCRAWL_PAGE_RESTRICT_OK;
				}

				webcrawl_process_page_result result = WEBCRAWL_PAGE_RESTRICT_OK;

				for(size_t n = 0; n < string_dup_size; n++) {
					//bitwise or the values so we note all restrictions
					//WEBCRAWL_PAGE_RESTRICT_ALL = WEBCRAWL_PAGE_RESTRICT_NOFOLLOW | WEBCRAWL_PAGE_RESTRICT_NOINDEX
					if(strcmp((char*)string_dup_array[n], "nofollow") == 0) {
						result |= WEBCRAWL_PAGE_RESTRICT_NOFOLLOW;
					}
					if(strcmp((char*)string_dup_array[n], "noindex") == 0) {
						result |= WEBCRAWL_PAGE_RESTRICT_NOINDEX;
					}
				}

				webcrawl_free_ustr_array(string_dup_array, string_dup_size);
				xmlFree(name); xmlFree(content);
				return result;
			}
			else if(strcmp((char*)name, "description") == 0) {
				//split every word
				webcrawl_split_ustr(content, ' ', &appageinfo->description_words, &appageinfo->description_words_amount, 0);
			}
			else if(strcmp((char*)name, "keywords") == 0) {
				//HTML spec says that keywords are comma separated
				//https://html.spec.whatwg.org/multipage/semantics.html#standard-metadata-names
				webcrawl_split_ustr(content, ',', &appageinfo->keywords, &appageinfo->keywords_amount, 0);
			}
			else if(!apstatus->has_author && strcmp((char*)name, "author") == 0) {
				size_t length_of_author = strlen((const char*)content);
				appageinfo->author = malloc((length_of_author+1)*sizeof(char));
				memcpy(appageinfo->author, content, length_of_author);
				appageinfo->author[length_of_author] = '\0';
				apstatus->has_author = true;
			}
			else {}
		}
		xmlFree(name);
		xmlFree(content);
	}
	else if(strcmp((char*)apnode->name, "title") == 0 && !apstatus->has_title) {
		xmlChar* title = xmlNodeGetContent(apnode);
		if(!title) {
			fprintf(stdout, "libxml allocation failed when getting page title.\n");
			exit(EXIT_FAILURE);
		}
		apstatus->has_title = true;

		size_t length_of_title = strlen((const char*)title);
		appageinfo->title = malloc((length_of_title+1)*sizeof(char));
		memcpy(appageinfo->title, title, length_of_title);
		appageinfo->title[length_of_title] = '\0';

		xmlFree(title);
	}
	//some websites use h1, some use h for main headers
	else if(
		(strcmp((char*)apnode->name, "h1") == 0 || strcmp((char*)apnode->name, "h") == 0) &&
		!apstatus->has_header
	) {
		xmlChar* header = xmlNodeGetContent(apnode);
		if(!header) {
			fprintf(stdout, "libxml allocation failed when getting page header.\n");
			exit(EXIT_FAILURE);
		}
		apstatus->has_header = true;

		size_t length_of_header = strlen((const char*)header);
		appageinfo->header1 = malloc((length_of_header+1)*sizeof(char));
		memcpy(appageinfo->header1, (const char*)header, length_of_header);
		appageinfo->header1[length_of_header] = '\0';

		xmlFree(header);
	}

	return WEBCRAWL_PAGE_RESTRICT_OK;
}

webcrawl_process_page_result internal_traverse_node_children(internal_page_data_status* apstatus, xmlNode* aproot, webcrawl_page_info* const appageinfo, webcrawl_queue* apqueue, const webcrawl_http_response_data* const apdata) {
	if(!aproot) return apstatus->result;
	if(apstatus->result == WEBCRAWL_PAGE_RESTRICT_ALL) return WEBCRAWL_PAGE_RESTRICT_ALL;

	apstatus->result = internal_evaluate_node(apstatus, aproot, appageinfo, apqueue, apdata); //get info about node

	//recursive solution is the best here
	for(xmlNode* node = aproot->children; node; node = node->next) {
		if(node) {
			//combine values (ALL is just all combinations bitwise or'd)
			apstatus->result |= internal_traverse_node_children(apstatus, node, appageinfo, apqueue, apdata); //traverse children
		}
	}

	return apstatus->result;
}

webcrawl_process_page_result webcrawl_process_page(const webcrawl_http_response_data* const apdata, webcrawl_page_info* const appageinfo, webcrawl_queue* apqueue) {
	appageinfo->url = (unsigned char*)strdup((char*)apdata->url);
	appageinfo->last_visited = time(NULL);

	htmlDocPtr document = htmlReadDoc(apdata->data, NULL, NULL, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
	if(!document) {
		fprintf(stdout, "libxml2 HTML Parsing failed. %s\n", xmlGetLastError()->message);
		exit(EXIT_FAILURE);
	}

	internal_page_data_status status;
	status.has_title = false;
	status.has_header = false;
	status.has_author = false;
	status.result = WEBCRAWL_PAGE_RESTRICT_OK;

	xmlNode* root;
	root = xmlDocGetRootElement(document);
	webcrawl_process_page_result result = internal_traverse_node_children(&status, root, appageinfo, apqueue, apdata);
	xmlFreeDoc(document);
	return result;
}
