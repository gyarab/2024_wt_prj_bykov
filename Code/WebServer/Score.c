#include "Score.h"

//ignoring accents
double_t webserver_string_equality(const char* ars1, const char* ars2) {
	if(ars1 == NULL || ars2 == NULL) { return 0.0; }

	size_t len1 = strlen(ars1);
	size_t len2 = strlen(ars2);
	if(len1 == 0 || len2 == 0) { return 0.0; } //ignore if empty strings

	size_t min_len = len1 < len2 ? len1 : len2;
	size_t max_len = len1 > len2 ? len1 : len2;

	size_t matching = 0;
	for(size_t i = 0; i < min_len; i++) {
		if(ars1[i] == ars2[i]) matching++;
	}

	return (double_t)matching/(double_t)max_len;
}

//header and title are more important, author and keywords are mostly ignored

#define WEBSERVER_ADDRESS_WEIGHT 10
#define WEBSERVER_HEADER_WEIGHT 5
#define WEBSERVER_TITLE_WEIGHT 5
#define WEBSERVER_AUTHOR_WEIGHT 1
#define WEBSERVER_KEYWORDS_WEIGHT 1
#define WEBSERVER_DESCRIPTION_WEIGHT 2

//URLs to and from act as tiebreak
#define WEBSERVER_URL_TO_WEIGHT 0.0001
#define WEBSERVER_URL_FROM_WEIGHT 0.0001

double_t webserver_calculate_score(webserver_page_info* apentry, const char* arquery) {
	//each piece of information has its own weight
	//if (almost) exact match - multiply by 10 in some cases - guarantee first result if definitely important

	unsigned char** split_query;
	size_t split_query_size;
	webserver_split_ustr((unsigned char*)arquery, ' ', &split_query, &split_query_size, 0);

	double_t paddress = webserver_string_equality((char*)apentry->url, arquery);
	if(paddress >= 0.97) paddress *= 10;
	double_t ptitle = webserver_string_equality((char*)apentry->title, arquery);
	if(ptitle >= 0.97) ptitle *= 10;

	//dont use multiplication for header and author, is often irrelevant to content (address or title shouldnt be)
	double_t pheader = webserver_string_equality((char*)apentry->header1, arquery);
	double_t pauthor = webserver_string_equality((char*)apentry->author, arquery);

	//multiple word sections - match with each word in query, add best percentage results

	double_t pkeywords = 0.0;
	if(apentry->keywords_amount != 0) {
		pkeywords = 1.0;

		for(size_t i = 0; i < split_query_size; i++) {
			double_t best_match_to_word = 0.0;
			for(size_t j = 0; j < apentry->keywords_amount; j++) {
				double_t temp_p = webserver_string_equality((char*)apentry->keywords[j], (char*)split_query[i]);
				if(temp_p > best_match_to_word) {
					best_match_to_word = temp_p;
				}
			}

			//multiply if equal
			if(best_match_to_word > 0.97) best_match_to_word *= 10;
			pkeywords += best_match_to_word;
		}

		if(split_query_size != 0) {
			pkeywords /= split_query_size;
		}
	}

	double_t pdescription = 0.0;
	if(apentry->description_words_amount != 0) {
		pdescription = 1.0;

		for(size_t i = 0; i < split_query_size; i++) {
			double_t best_match_to_word = 0.0;
			for(size_t j = 0; j < apentry->description_words_amount; j++) {
				double_t temp_p = webserver_string_equality((char*)apentry->description_words[j], (char*)split_query[i]);
				if(temp_p > best_match_to_word) {
					best_match_to_word = temp_p;
				}
			}

			//multiply if equal
			if(best_match_to_word > 0.97) best_match_to_word *= 10;
			pdescription += best_match_to_word;
		}

		if(split_query_size != 0) {
			pdescription /= split_query_size;
		}
	}

	webserver_free_ustr_array(split_query, split_query_size);

	//just add percentages with weights + tiebreak with links (miniscule, otherwise abused by URL "hubs") and divide by length of query
	//compensate for length of query an higher multiword matching scores, has an effect
	return
		(WEBSERVER_ADDRESS_WEIGHT*paddress)+
		(WEBSERVER_HEADER_WEIGHT*pheader)+
		(WEBSERVER_TITLE_WEIGHT*ptitle)+
		(WEBSERVER_AUTHOR_WEIGHT*pauthor)+
		(WEBSERVER_KEYWORDS_WEIGHT*pkeywords)+
		(WEBSERVER_DESCRIPTION_WEIGHT*pdescription)+
		(WEBSERVER_URL_FROM_WEIGHT*apentry->amount_links_from)+
		(WEBSERVER_URL_TO_WEIGHT*apentry->amount_links_to) / strlen(arquery);
}
