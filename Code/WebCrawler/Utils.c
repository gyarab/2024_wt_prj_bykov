#include "Utils.h"

void webcrawl_split_ustr(const unsigned char* arstring, const unsigned char ardelimiter, unsigned char*** apprstringarray, size_t* aarraysize, size_t amaxsize) {
	if(!arstring) { return; }

	*apprstringarray = NULL;
	*aarraysize = 0;

	size_t maxsplits = SIZE_MAX;
	//if 0 parameter ignored
	if(amaxsize != 0) {
		//array size = maxsplits+1
		maxsplits = amaxsize-1;
	}

	//get size of string and array to preallocate pointers
	size_t size_of_string = strlen((char*)arstring);
	if(size_of_string == 0) {
		return;
	}

	size_t offset = 0;
	size_t lastoffset = 0;
	size_t sections = 0;
	size_t sections_allocated = 10;
	*apprstringarray = malloc(sections_allocated*sizeof(unsigned char*));

	//find each delimiter
	while(true) {
		char* delim = strchr((char*)&arstring[offset], ardelimiter);
		size_t segment_size = 0;
		//both +1 - sections has +1 because we add new element (which results in array size) and maxsplits+1 is the array size
		if(!delim || sections+1 == maxsplits+1) {
			//add rest of string
			segment_size = (size_of_string - offset);
			lastoffset = offset;
		}
		else {
			//add one section
			lastoffset = offset;
			offset = delim - (char*)arstring + 1;
			segment_size = (offset - lastoffset) - 1; //+1 and -1 compensate each other
		}

		//assign and init memory for section
		if(sections == sections_allocated) {
			sections_allocated *= 2;
			*apprstringarray = realloc(*apprstringarray, sections_allocated*sizeof(unsigned char*)); //allocate pointer(s)

		}
		(*apprstringarray)[sections] = malloc(segment_size+1); //allocate memory for segment
		memcpy((*apprstringarray)[sections], &arstring[lastoffset], segment_size); //copy data to segment
		(*apprstringarray)[sections][segment_size] = '\0';

		sections++;
		if(!delim || sections == maxsplits+1) break; //break out of loop at last element
	}

	*apprstringarray = realloc(*apprstringarray, sections*sizeof(unsigned char*)); //shrink to needed size
	(*aarraysize) = sections;
}
void webcrawl_free_ustr_array(unsigned char** aprstringarray, const size_t aarraysize) {
	if(!aprstringarray || aarraysize == 0) {
		return; //empty array
	}

	for(size_t n = 0; n < aarraysize; n++) {
		free(aprstringarray[n]);
	}
	free(aprstringarray);
	aprstringarray = NULL;
}

unsigned char* webcrawl_combine_strings(unsigned char** apstrings, const size_t aamount) {
	if(aamount == 0 || !apstrings) return NULL;

	size_t* sizes = calloc(aamount, sizeof(size_t));
	size_t complete_size = 0;
	for(size_t n = 0; n < aamount; n++) {
		sizes[n] = strlen((const char*)apstrings[n]);
		complete_size += sizes[n];
	}

	//+1 for NT, aamount-1 for splitting bytes
	unsigned char* string = malloc((complete_size+(aamount-1)+1));
	size_t offset = 0;

	for(size_t n = 0; n < aamount; n++) {
		memcpy(&string[offset], apstrings[n], sizes[n]);
		offset += sizes[n];
		string[offset] = WEBCRAWL_DELIMITER_BYTE;
		offset++;
	}

	string[complete_size+(aamount-1)] = '\0';

	free(sizes);
	return string;
}

void webcrawl_remove_space_ustr(unsigned char** aprstring) {
	size_t size_of_string = strlen((char*)(*aprstring));
	unsigned char* local_ref = malloc(size_of_string);
	size_t local_ref_increment = 0;
	//copying byte-by-byte is inefficent: is this becomes a problem, optimize it
	for(size_t n = 0; n < size_of_string; n++) {
		if(!isblank((*aprstring)[n])) {
			local_ref[local_ref_increment] = (*aprstring)[n];
			local_ref_increment++;
		}
	}
	free(*aprstring);
	*aprstring = realloc(local_ref, local_ref_increment+1);
	(*aprstring)[local_ref_increment] = '\0';
}

size_t webcrawl_math_min(const size_t aa, const size_t ab) {
	return aa < ab ? aa : ab;
}
size_t webcrawl_math_max(const size_t aa, const size_t ab) {
	return aa > ab ? aa : ab;
}

const unsigned char chttps_comparison[8] = {'h', 't', 't', 'p', 's', ':', '/', '/'};
const unsigned char chttp_comparison[8] = {'h', 't', 't', 'p', ':', '/', '/', '\0'};
uint64_t cmask_http = (uint64_t)0b11111111 << 56;

//returns offset to skip http(s) protocol
size_t internal_get_protocol_offset(const unsigned char* arurl) {
	//shortest possible with protocol: http://a.bb, length 11
	if(strlen((char*)arurl) < 11) return 0;
	if(((*(uint64_t*)arurl) | cmask_http) == ((*(uint64_t*)chttp_comparison) | cmask_http)) {
		return 7; //http://*
	}
	else if(*(uint64_t*)arurl == *(uint64_t*)chttps_comparison) {
		return 8; //https://*
	}
	return 0;
}

//webcrawl_get_domain and webcrawl_get_path are basically the same - we just copy different parts of the string to the result buffer

unsigned char* webcrawl_get_domain(const unsigned char* araddr) {
	size_t front_offset = 0;
	uint32_t length = 0;

	//decode URL - handle can be NULL since it is not used according to docs
	unsigned char* dirty_url = (unsigned char*)curl_easy_unescape(NULL, (char*)araddr, 0, (int32_t*)&length);
	front_offset = internal_get_protocol_offset(dirty_url); //get protocol offset

	//shortest possible with www: www.a.bb
	if(length >= 8) {
		uint32_t www_http = *(uint32_t*)"www.";
		if(*((uint32_t*)&dirty_url[front_offset]) == www_http) {
			front_offset += 4; //www.
		}
	}

	char* section = strchr((char*)&dirty_url[front_offset], '/'); //first slash after protocol splits domain and pages
	if(section != NULL) {
		length = section - (char*)dirty_url;
	}

	unsigned char* result = malloc((length-front_offset+1));
	memcpy(result, &dirty_url[front_offset], length-front_offset);
	result[length-front_offset] = '\0';
	curl_free(dirty_url);
	return result;
}

//copy of get_domain except we
unsigned char* webcrawl_get_path(const unsigned char* araddr) {
	size_t front_offset = 0;
	uint32_t length = 0;

	unsigned char* dirty_url = (unsigned char*)curl_easy_unescape(NULL, (char*)araddr, 0, (int32_t*)&length);
	front_offset = internal_get_protocol_offset(dirty_url);//get protocol offset

	char* section = strchr((char*)&dirty_url[front_offset], '/'); //first slash after protocol splits domain and pages
	if(section != NULL) {
		front_offset = section - (char*)dirty_url;
	}

	unsigned char* result = malloc((length-front_offset+1));
	memcpy(result, &dirty_url[front_offset], length-front_offset);
	result[length-front_offset] = '\0';
	curl_free(dirty_url);
	return result;
}

//free returned pointer, araddr is read only
unsigned char* webcrawl_clean_address(const unsigned char* araddr) {
	size_t front_offset = 0;
	uint32_t length;
	unsigned char* dirty_url = (unsigned char*)curl_easy_unescape(NULL, (char*)araddr, 0, (int32_t*)&length);

	//remove protocol: DNS will figure it out (either http or https)

	front_offset = internal_get_protocol_offset(dirty_url); //get protocol offset

	//remove URL sections (everything after #)
	//we just ignore it and simply will not copy it to result

	char* section = strchr((char*)dirty_url, '#');
	if(section != NULL) {
		length = section - (char*)dirty_url;
	}

	//remove .html or .htm extension if present (and only if at end)

	//shortest possible with parameter: a.b?c=d, length 7
	if(length >= 7) {
		const unsigned char cextension[8] = {'.', 'h', 't', 'm', 'l', '\0', '\0', '\0'};
		const size_t chtm_mask = 0b11111111 << 24;

		uint64_t* compare = malloc(sizeof(uint64_t));
		*compare = 0; //fill with zeroes (should be faster than a function call)
		memcpy(compare, &dirty_url[length-5], 5); //copy chars that potentially house extension

		if(*compare == *((uint64_t*)&cextension)) {
			//has .html extension
			length -= 5;
		}
		else if((*compare | chtm_mask) == (*((uint64_t*)&cextension) | chtm_mask)) {
			//has .htm extension
			length -= 4;
		}

		free(compare);
	}

	unsigned char* result = malloc((length-front_offset+1));
	memcpy(result, &dirty_url[front_offset], length-front_offset);
	result[length-front_offset] = '\0';
	curl_free(dirty_url);
	return result;
}

//free returned ptr!
unsigned char* webcrawl_join_address(const unsigned char* arfutureaddr, const unsigned char* arcurrentaddr) {
	//arcurrentaddr = our current address
	//arfutureaddr = changes to current address which lead to future address

	size_t future_length = strlen((const char*)arfutureaddr);
	if(arfutureaddr[0] == '#') return NULL; //section of address, ignore
	size_t current_length = strlen((const char*)arcurrentaddr);
	unsigned char* result = NULL;

	//absolute address
	//fix no. 2 for wikipedia, which uses //lng.wikipedia.org for some reason
	if(internal_get_protocol_offset(arfutureaddr) != 0 || (arfutureaddr[0] == '/' && arfutureaddr[1] == '/')) {
		//external website address - remove // or protocol
		unsigned char* noproto = (unsigned char*)strstr((char*)arfutureaddr, "//") + 2; //remove // and, by extension, the protocol - add length of it, strstr points to beginning
		result = (unsigned char*)strdup((char*)noproto);
		return result;
	}

	//relative address to current page/site

	unsigned char* clean_current = webcrawl_clean_address(arcurrentaddr);
	unsigned char* clean_future = webcrawl_clean_address(arfutureaddr);

	//remove parameters from both future and current
	char* paramptr = strchr((char*)clean_current, '?');
	if(paramptr) {
		clean_current[paramptr - (char*)clean_current] = '\0'; //removing parameters by terminating string early
	}
	paramptr = strchr((char*)clean_future, '?');
	if(paramptr) {
		clean_future[paramptr - (char*)clean_future] = '\0'; //removing parameters by terminating string early
	}

	unsigned char** split_future = NULL;
	size_t split_future_size = 0;
	webcrawl_split_ustr(clean_future, '/', &split_future, &split_future_size, 0);

	unsigned char** split_current = NULL;
	size_t split_current_size = 0;

	if(arfutureaddr[0] == '/' || (arfutureaddr[0] != '?' && arfutureaddr[0] != '.')) {
		//if begins with / OR begins with some letter: relative to domain - start at domain
		//fix for https://vyzkum.gov.cz/FrontPristupnost.aspx czech government website
		split_current = malloc(sizeof(unsigned char*));
		split_current[0] = webcrawl_get_domain(clean_current);
		split_current_size = 1;
	}
	else {
		//relative to current page
		webcrawl_split_ustr(clean_current, '/', &split_current, &split_current_size, 0);
	}

	size_t split_current_allocated_size = split_current_size;
	free(clean_current);
	free(clean_future);

	result = malloc((future_length+current_length)+2); //cannot be larger than this, will be shortened later (current/future+null term)

	for(size_t n = 0; n < split_future_size; n++) {
		//navigate to upper directory: remove last directory of current address
		if(strcmp((char*)split_future[n], "..") == 0) {
			if(split_current_size <= 1) {
				//cannot go one step under domain - stop
				continue;
			}
			split_current_size--;
			free(split_current[split_current_size]);
		}
		//current directory: ignore
		else if(strcmp((char*)split_future[n], ".") == 0) {
			continue;
		}
		//new directory: add directory to current address
		else {
			if(split_current_size == split_current_allocated_size) {
				split_current_allocated_size *= 2;
				//allocating ahead: realloc calls are expensive
				split_current = realloc(split_current, split_current_allocated_size*sizeof(unsigned char*));
			}

			//copy directory from future to current
			size_t length_of_dir = strlen((const char*)split_future[n]); //we use min here sinze cutoff might be SIZE_MAX
			split_current[split_current_size] = malloc((length_of_dir+1));
			memcpy(split_current[split_current_size], split_future[n], length_of_dir);
			split_current[split_current_size][length_of_dir] = '\0';
			split_current_size++;
		}
	}

	//convert array to string using memcpy
	size_t offset = 0;
	for(size_t n = 0; n < split_current_size; n++) {
		size_t str_length = strlen((const char*)split_current[n]);
		if(str_length == 0) continue;
		memcpy(&result[offset], split_current[n], str_length);
		result[offset + str_length] = '/';
		offset += (str_length + 1);
	}

	result = realloc(result, (offset+1)); //save memory by shrinking it to correct size
	result[offset] = '\0';

	webcrawl_free_ustr_array(split_future, split_future_size);
	webcrawl_free_ustr_array(split_current, split_current_size);

	return result;
}

const size_t cinternal_time_dividers[] = {60, 60, 60, 24, 30, 365};
const unsigned char cinternal_time_units[] = "smhdMy";

webcrawl_time_output webcrawl_get_correct_time_unit(const size_t aseconds) {
	webcrawl_time_output result;
	result.value = aseconds;

	size_t n;
	for(n = 0; n < sizeof(cinternal_time_dividers); n++) {
		if(result.value > cinternal_time_dividers[n] - 1) {
			result.value = (result.value - (result.value % cinternal_time_dividers[n])) / cinternal_time_dividers[n];
		}
		else break;
	}

	result.unit = cinternal_time_units[n];

	return result;
}
