#ifndef WEBCRAWL_FILE_UTILS
#define WEBCRAWL_FILE_UTILS
#include "Include.h"

//splits a string into multiple strings, if amaxsize if 0 parameter is ignored, function always returns at least 1 element
void webcrawl_split_ustr(const unsigned char* arstring, const unsigned char ardelimiter, unsigned char*** apprstringarray, size_t* aarraysize, size_t amaxsize);
//frees and array of unsinged char strings
void webcrawl_free_ustr_array(unsigned char** aprstringarray, const size_t aarraysize);

unsigned char* webcrawl_combine_strings(unsigned char** apstrings, const size_t aamount);

//removes blank characters in it, modifies string
void webcrawl_remove_space_ustr(unsigned char** aprstring);

size_t webcrawl_math_min(const size_t aa, const size_t ab);
size_t webcrawl_math_max(const size_t aa, const size_t ab);

//gets domain from URL (removes path and other stuff)
unsigned char* webcrawl_get_domain(const unsigned char* araddr);
//gets path and parameters from URL (removes domain)
unsigned char* webcrawl_get_path(const unsigned char* araddr);

//removes protocol, URL sections (#...) from address, does NOT remove parameters (?a=b)
unsigned char* webcrawl_clean_address(const unsigned char* araddr);
//joins 2 addresses together
unsigned char* webcrawl_join_address(const unsigned char* arfutureaddr, const unsigned char* arcurrentaddr);

typedef struct {
	size_t value;
	unsigned char unit;
} webcrawl_time_output;

//gets most appropriate unit (s = second/m = minute/h = hour/d = day/M = month/y = year)
webcrawl_time_output webcrawl_get_correct_time_unit(const size_t aseconds);

#endif
