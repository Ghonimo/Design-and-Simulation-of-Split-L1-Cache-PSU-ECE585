#include "header.h"

/* Snooping Function
 * in response to n = 4 in the trace file
 * This function is a simulaion of L2 snooping on L1
 * During this snooping operation, L2 invalidates all the MESI bits.
 * For MESI protocol of the snooping operation, we have:
 * If we're in 'E' we go to 'I', if we're in 'S' go to 'I', if in 'I' stay in 'I', if in 'M' go to 'I'
*/
void snoop(unsigned int addr) {

	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);				// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;		// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set

	for (int i = 0; i < 8; ++i) {						// Look for a matching tag in the data cache
		if (L1_data[i][set].tag_bits == tag) {
				L1_data[i][set].MESI_char = 'I';		// Change MESI bit to Invalid
				L1_data[i][set].tag_bits = tag;
				L1_data[i][set].set_bits = set;
				L1_data[i][set].address = addr;

			if (mode == 1) {							// Simulating a snoop return data communication with L2
				cout << "[Snoop] Return data to L2 " << hex << addr << endl;
			}
		}
	}
	return;
}