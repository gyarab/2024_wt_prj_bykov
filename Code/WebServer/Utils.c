#include "Utils.h"

void webserver_split_ustr(const unsigned char* arstring, const unsigned char ardelimiter, unsigned char*** apprstringarray, size_t* aarraysize, size_t amaxsize) {
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
void webserver_free_ustr_array(unsigned char** aprstringarray, const size_t aarraysize) {
	if(!aprstringarray || aarraysize == 0) {
		return; //empty array
	}

	for(size_t n = 0; n < aarraysize; n++) {
		free(aprstringarray[n]);
	}
	free(aprstringarray);
	aprstringarray = NULL;
}
