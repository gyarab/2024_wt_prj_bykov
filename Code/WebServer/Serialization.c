#include "Serialization.h"

char* webserver_serialize_query_result(webserver_page_info* apinfo) {
	//calculate size of result
	size_t result_size = 1; // ENTRY_NEW byte at the end

	//check if strings are NULL
	if(apinfo->title) result_size += strlen((char*)apinfo->title);
	if(apinfo->author) result_size += strlen((char*)apinfo->author);
	if(apinfo->url) result_size += strlen((char*)apinfo->url);

	result_size += 3; // ENTRY_SPLIT byte will be present anyway, even if string is NULL or empty

	for(size_t i = 0; i < apinfo->description_words_amount; i++) {
		result_size += strlen((char*)apinfo->description_words[i]);
		result_size ++; //accounts for spaces OR ending character ENTRY_NEW
	}

	//allocate memory, +1 for null terminator
	char* result = malloc(result_size + 1);

	//copy stuff

	size_t offset = 0, size = 0;

	if(apinfo->title) {
		size = strlen((char*)apinfo->title);
		memcpy(&result[offset], (char*)apinfo->title, size);
		offset += size;
	}
	result[offset] = WEBSERVER_ENTRY_SPLIT;
	offset++;

	if(apinfo->author) {
		size = strlen((char*)apinfo->author);
		memcpy(&result[offset], (char*)apinfo->author, size);
		offset += size;
	}
	result[offset] = WEBSERVER_ENTRY_SPLIT;
	offset++;

	if(apinfo->url) {
		size = strlen((char*)apinfo->url);
		memcpy(&result[offset], (char*)apinfo->url, size);
		offset += size;
	}
	result[offset] = WEBSERVER_ENTRY_SPLIT;

	//copy description words
	for(size_t i = 0; i < apinfo->description_words_amount; i++) {
		offset++;
		size = strlen((char*)apinfo->description_words[i]); //TODO LATER move sizes into array, read from it to not call strlen twice
		memcpy(&result[offset], (char*)apinfo->description_words[i], size);
		offset += size;
		result[offset] = ' ';
	}

	offset++;

	//terminate entry and string
	result[offset] = WEBSERVER_ENTRY_NEW; //overwrite space
	result[offset + 1] = '\0';

	return result;
}
