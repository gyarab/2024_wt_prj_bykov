#ifndef WEBCRAWL_FILE_RULES
#define WEBCRAWL_FILE_RULES
#include "Database.h"

//saves robots.txt into a DRD entry and visits a sitemap if present
void webcrawl_get_domain_info(CURL* aphandle, unsigned char* ardomain, webcrawl_drd_entry* apdrdentry, webcrawl_queue* apqueue);

//check if address allowed by robots.txt (data inside drd entry), return true if so, false otherwise
bool webcrawl_check_if_allowed(unsigned char* araddress, webcrawl_drd_entry* apdrdentry);

#endif
