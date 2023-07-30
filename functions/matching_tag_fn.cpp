#include "header.h"

/* matching_tag function
* This function looks for a matching tag in the cache. 
* If it finds a matching tag, it returns the way in which we have this matching tag. 
* If we have matching tag, we have a hit, except if the MESI char is "I" invalid. 
* This function takes in the tag, the set, and a char flag to indicate whether we're 
* looking in the data or the instructions cache. 
* Inputs: tag, set, and which_chace flag. 
* Outputs: cache way, or -1 if we don't have a match
 */
int matching_tag(unsigned int tag, unsigned int set, char which_cache) {
	int i = 0;
	if (which_cache == 'D'|| which_cache == 'd') {		// if flag is 'D' We're searching for a matching tag in the L1 data cache
		// search the data cache for matching tags
	while (L1_data[i][set].tag_bits != tag) {
		i++;
		if (i > 7) {				// We have 8 ways in the data cache 0 through 7
			return -1;				// return -1 to imply that we don't have a matching tag in the data cache. 
		}
	}
	}

	else {							// if flag is 'I' We're searching for a matching tag in the L1 data cache
		int i = 0;
		// Check for matching tags;
		while (L1_inst[i][set].tag_bits != tag) {
			i++;
			if (i > 3) {			// We have 4 ways in the instruction cache 0 through 3
				return -1;			// return -1 to imply that we don't have a matching tag in the instruction cache.
			}
		}
	}
	return i;						// If we have a matching tag, return the way in which we have the matching tag. 
}

