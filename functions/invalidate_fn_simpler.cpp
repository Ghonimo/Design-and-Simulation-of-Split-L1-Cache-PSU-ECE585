#include "header.h"

/* Invalidate Operation
*  This function will change the MESI state to 'I' Invalid
*  Regardless of what it was before
*  This function is a simulation of an invalidate command from L2
*  The function takes in the address needed to be invalidated, doesn't output anything
*/
void invalidate(unsigned int addr) {

	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);				// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;		// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set

	// Search the L1_data cache for the matching tag
	for (int i = 0; i < 8; ++i) {
		if (L1_data[i][set].tag_bits == tag) {

			L1_data[i][set].MESI_char = 'I';	// change the MESI Protocol to the Invalid state
			L1_data[i][set].tag_bits = tag;
			L1_data[i][set].set_bits = set;
			L1_data[i][set].address = addr;
		}
		// There's no else. If we don't have a matching tag, do nothing
	}
	return;
}