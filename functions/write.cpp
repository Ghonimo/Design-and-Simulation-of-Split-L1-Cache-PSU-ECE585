#include "header.h"


/* Data write Function
 * in response to n = 1 in the trace file
 * This function attempts to write a an address/line from the data cache
 * If we have a miss, we place the line in an empty line
 * if no empty lines, we check for an a line in the invalid state, evict it and place the line there
 * if not invalid states, we evict the LRU and place the line in that way
 * This function takes in an address, and doesn't need to return anything
 * For the data read MESI protocol we have:
 * If we're in 'M' we stay in 'M', if we're in 'E' we go to 'M', if in 'S' we go to 'E', if in 'I' go to 'E'
 */

void write(unsigned int addr) {


	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);				// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;		// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set
	bool empty_flag = 0;												// boolean flag for empty way in the cache, if 1, we have an empty way
	int way = -1;														// way in the cache set. Initialized with an invalid way value

	statistics.data_write++;							// Increment the number of write for the data cache

	way = matching_tag(tag, set, 'D');				// Look for a matching tag already in the data cache
	if (way >= 0) {									// if we have a matching tag, then we have an data cache hit! (unless invalid MESI state)
		switch (L1_data[way][set].MESI_char) {
		case 'M':									// in we're in the modified state
			statistics.data_hit++;					// Incremenet the data hit counter (We have a new hit!)
			L1_data[way][set].MESI_char = 'M';		// stay in the modified state
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');		// Update the data cache LRU count
			break;

		case 'E':									// if we're writing in an exclusive state, the data will be modified
			statistics.data_hit++;					// Incremenet the data hit counter (We have a new hit!)
			L1_data[way][set].MESI_char = 'M';		// go to the modified state
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');		// Update the data cache LRU count
			break;

		case 'S':									// if we're writing in a shared state, the writen data will be exclusive
			statistics.data_hit++;					// Incremenet the data hit counter (We have a new hit!)
			L1_data[way][set].MESI_char = 'E';		// go to the exclusive state
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');		// Update the data cache LRU count
			break;
				
		case 'I':									// if we are in the invalid state, we'll have a miss
			statistics.data_miss++;					// Incremenet the data miss counter 
			L1_data[way][set].MESI_char = 'E';		// go to the exclusive state
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');		// Update the data cache LRU count
			break;
		}
	}

	else {												// Data Cache Miss
		statistics.data_miss++;							// Incremenet the data miss counter 

		if (mode == 1) {								// Simulating a cache Read For Ownership communication from L2
			cout << " [Data] Read for Ownership from L2 " << hex << addr << endl;
		}

		for (int i = 0; way < 0 && i < 8; ++i) {		// First, check if we have any empty lines
			if (L1_data[i][set].tag_bits == EMPTY_TAG) {
				way = i;								// return the way that has an empty line
				empty_flag = 1;							// if we have an empty line, toggle the empty_flag to high
			}
		}

		if (way >= 0) {							// if we have an empty line, place the read data in it
			L1_data[way][set].MESI_char = 'M';			// go to the modified state
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');			// Update the data cache LRU order/count

			if (mode == 1) {							// Simulating an iniial write through communication from L2
				cout << "[Data Write Through] Write to L2: we have a data Cache Miss " << hex << addr << endl;
			}
		}

		else {											// if we don't have any empty lines, we need to evict the LRU line. 
			if (mode == 1) {							// Simulating an write back communication from L2
				cout << "[Data Write back] Write to L2: we have a data Cache Miss " << hex << addr << endl;
			}

			for (int n = 0; n < 8; ++n) {			
				if (L1_data[n][set].MESI_char == 'I') {	// if we have an invalid way, we evict it
					way = n;							// return the way that has an Invalid line
				}		
				else {	
					way = -1;							// set the way to -1, meaning no invalid lines
				}
			}
			if (way < 0) {								//If we don't have any invalid lines, 
				way = find_LRU(set, 'D');				// Find the LRU way in the data cache
				if (way >= 0) {							// if we have a way that's 0, (LRU in the data cache, we use it)
					L1_data[way][set].MESI_char = 'M';	// go to the modified state
					L1_data[way][set].tag_bits = tag;
					L1_data[way][set].set_bits = set;
					L1_data[way][set].address = addr;

					L1_LRU(way, set, empty_flag, 'D');	// update the L1 data cache LRU bits
				}
			}
			else { 										// if we have an invalid way, we evict it 
				L1_data[way][set].MESI_char = 'M';		// go to the modified state
				L1_data[way][set].tag_bits = tag;
				L1_data[way][set].set_bits = set;
				L1_data[way][set].address = addr;

				L1_LRU(way, set, empty_flag, 'D');	// update the L1 data cache LRU bits
			}

		}
	}
	return;
}
