#include "header.h"

/*
*	When this function is called it clears the cache to reset it to the intitial empty state
*	and clears all the statisticsitics.
 */
void clear_cache() {

	cout << "\n\t Clear the cache to the initial state and reset the statistics" << endl;

	// Clear and reset the data cache
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 16384; ++j) {
			L1_data[i][j].MESI_char = 'I';			// Reset the MESI protocol to the Invalid state
			L1_data[i][j].LRU_bits = 0;				// reset the LRU bits to 0
			L1_data[i][j].tag_bits = EMPTY_TAG;		// We are using the value 4096 to indicate an empty tag (since 0 - 4095 are used)
			L1_data[i][j].set_bits = 0;				// reset the set bits to 0
			L1_data[i][j].address = 0;				// reset the address to 0
		}
	}


	// Clearing the instruction cache
	for (int n = 0; n < 4; ++n) {
		for (int m = 0; m < 16384; ++m) {
			L1_inst[n][m].MESI_char = 'I';		// Reset the MESI protocol to the Invalid state
			L1_inst[n][m].LRU_bits = 0;			// reset the LRU bits to 0
			L1_inst[n][m].tag_bits = EMPTY_TAG;		// We are using the value 4096 to indicate an empty tag (since 0 - 4095 are used)
			L1_inst[n][m].set_bits = 0;			// reset the set bits to 0
			L1_inst[n][m].address = 0;			// reset the address to 0
		}	
	}

	// Clear the instruction cache statistics and reset their values to 0
	statistics.inst_read = 0;
	statistics.inst_hit = 0;
	statistics.inst_miss = 0;
	statistics.inst_hit_ratio = 0.0;

	// Clear the data cache statistics and reset their values to 0
	statistics.data_read = 0;
	statistics.data_write = 0;
	statistics.data_hit = 0;
	statistics.data_miss = 0;
	statistics.data_hit_ratio = 0.0;

	return;
}
