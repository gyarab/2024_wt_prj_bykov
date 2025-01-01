#ifndef WEBCRAWL_FILE_PROCESSING
#define WEBCRAWL_FILE_PROCESSING
#include "Rules.h"

//process page

typedef enum {
	WEBCRAWL_PAGE_RESTRICT_OK       = 0,
	WEBCRAWL_PAGE_RESTRICT_NOFOLLOW = 1 << 0,
	WEBCRAWL_PAGE_RESTRICT_NOINDEX  = 1 << 1,
	WEBCRAWL_PAGE_RESTRICT_ALL      = WEBCRAWL_PAGE_RESTRICT_NOFOLLOW | WEBCRAWL_PAGE_RESTRICT_NOINDEX,
} webcrawl_process_page_result;

webcrawl_process_page_result webcrawl_process_page(
	const webcrawl_http_response_data* const apdata,
	webcrawl_page_info* const appageinfo,
	webcrawl_queue* apqueue
);

#endif
