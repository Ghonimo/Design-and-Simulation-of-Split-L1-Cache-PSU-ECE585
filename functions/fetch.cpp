#include "header.h"

/* Instruction Fetch Function
 * in response to n = 2 in the trace file
 * This function attempts to read/fetch a an address/line from the instruction cache
 * If we have a miss, we place the instruction in an empty line
 * if no empty lines, we check for an a line in the invalid state, evict it and place the line there
 * if not invalid states, we evict the LRU and place the line in that way
 * This function takes in an address, and doesn't need to return anything
 * For the instruction fetch MESI protocol we have: 
 * If we're in 'E' we go to 'S', if we're in 'S' stay in 'S', if in 'M' stay in 'M', if in 'I' go to 'S'
 */
void fetch_inst(unsigned int addr) {

	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);			// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;	// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set
	bool empty_flag = 0;											// boolean flag for empty way in the cache, if 1, we have an empty way
	int way = -1;													// way in the cache set. Initialized with an invalid way value

	statistics.inst_read++;								// Increment the number of reads for the instructions cache
	way = matching_tag(tag, set,'I');					// Look a matching tag already in the instruction cache

	if (way >= 0) {										// if we have a matching tag, then we have an instruction cache hit! (unless invalid MESI state)
		switch (L1_inst[way][set].MESI_char) {			// FSM implementing the MESI protocol
		case 'M':										// If we are in the modified state
			statistics.inst_hit++;						// We have a valid hit, increment the instruction cache hit counter
			L1_inst[way][set].MESI_char = 'M';			// We remain in the modified MESI state
			L1_inst[way][set].tag_bits = tag;
			L1_inst[way][set].set_bits = set;
			L1_inst[way][set].address = addr;

			L1_LRU(way, set, empty_flag,'I');			// Update the instruction cache LRU count

		case 'E':										// If we are in the modified state, another processor is fetching
			statistics.inst_hit++;						// We have a valid hit, increment the instruction cache hit counter
			L1_inst[way][set].MESI_char = 'S';			// Move the Shared state
			L1_inst[way][set].tag_bits = tag;
			L1_inst[way][set].set_bits = set;
			L1_inst[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'I');			// Update the instruction cache LRU count


		case 'S':										// If we are in the shared state
			statistics.inst_hit++;						// We have a valid hit, increment the instruction cache hit counter
			L1_inst[way][set].MESI_char = 'S';			// We remain in the modified MESI state
			L1_inst[way][set].tag_bits = tag;
			L1_inst[way][set].set_bits = set;
			L1_inst[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'I');			// Update the instruction cache LRU count
			break;

		case 'I':										// If we are in the invalid state, we have a miss even though the tag exists
			statistics.inst_miss++;						// Increment the instruction cache miss counter
			L1_inst[way][set].MESI_char = 'S';			// Move the Shared state, another processor is doing the fetch
			L1_inst[way][set].tag_bits = tag;
			L1_inst[way][set].set_bits = set;
			L1_inst[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'I');			// Update the instruction cache LRU count

			if (mode == 1) {							// Simulating the cache fetch instruction communication with L2
				cout << " [Instruction] Read from L2: L1 cache miss, obtain instruction form L2 " << hex << addr <<'\n' << endl;
			}
			break;
		}
	}

	else {							// if we don't have a matching tag, we have an instruction cache miss
		statistics.inst_miss++;						// Increment the instruction cache miss counter

		for (int i = 0; way < 0 && i < 4; ++i) { 	// First, check if we have any empty lines
			if (L1_inst[i][set].tag_bits == EMPTY_TAG) {
				way = i;							// return the way that has an empty line
				empty_flag = 1;						// if we have an empty line, toggle the empty_flag to high
			}
		}
		if (way >= 0) {								// if we have an empty line, place the fetched instruction in it
			L1_inst[way][set].MESI_char = 'E';		// and mark it exclusive, it's the only line with this instruction
			L1_inst[way][set].tag_bits = tag;
			L1_inst[way][set].set_bits = set;
			L1_inst[way][set].address = addr;
		
			L1_LRU(way, set, empty_flag, 'I');		// Update the instruction cache LRU order/count

			if (mode == 1) {						// Simulating the cache fetch instruction communication with L2
				cout << " [Instruction] Read from L2: L1 cache miss, obtain instruction form L2 " << hex << addr << '\n' << endl;
			}
		}

		else {						// if we don't have any empty lines, we need to evict the LRU line. 
			if (mode == 1) {
				cout << " [Instruction] Read from L2: L1 cache miss, obtain intsruction from L2 " << hex << addr << '\n' << endl;
			}
			
			for (int n = 0; n < 4; ++n) {
				if (L1_inst[n][set].MESI_char == 'I') { // Since we don't have empty lines, we check for a way with an invalid lines to evict
					way = n;
				}
				else {
					way = -1;		// if we don't have any invalid instruction cache lines, flag the  way with "-1"
				}
			}
			if (way < 0) {			//If we don't have any invalid lines, 
				way = find_LRU(set,'I');				// Find the LRU way in the instruction cache
				if (way >= 0) {							// if we have a way that's 0, (LRU in the instruction cache, we use it)
					L1_inst[way][set].MESI_char = 'E';	// the instruction here will be new, and exclusive to this cache
					L1_inst[way][set].tag_bits = tag;
					L1_inst[way][set].set_bits = set;
					L1_inst[way][set].address = addr;

					L1_LRU(way, set, empty_flag,'I');	// update the L1 instruction cache LRU bits
				}
			}
			else { 				// if we have an invalid way, we evict it and 
				L1_inst[way][set].MESI_char = 'E';		// the instruction here will be new, and exclusive to this cache
				L1_inst[way][set].tag_bits = tag;
				L1_inst[way][set].set_bits = set;
				L1_inst[way][set].address = addr;

				L1_LRU(way, set, empty_flag,'I');
			}
		}
	}
	return;
}
