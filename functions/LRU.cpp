#include "header.h"

/*
	This function updates the LRU bits for the Data Cache  and the instruction caches
	It takes 4 inputs. the cache way, the set, an empty cache flag, and a which_way char flag
	if the which_way char flag is 'D' or 'd', we're operating on the data cache
	if the which_way char flag is 'I' or 'i', we're operating on the instruction cache
	First, we check if a way is empty, if it's empty we put the data or instruction in it 
	and make it the MRU, my setting its LRU bits to 111 (0x7) if data, or 11 (0x3) if instruction
	If a way is NOT empty, we compare LRU bits of the current set with the LRU bits of the other sets
	this function updates the LRU, and doesn't retun anything. (no output)

	*/


void L1_LRU(unsigned int way, unsigned int set, bool empty_flag,char which_cache) {
	switch (which_cache) {
	case ('d'):
	case ('D'): 	// We're in the L1_data cache if which_chache is 'D' or 'd'
		// If the way is empty, we use it and decrement every way less than this way.
		if (empty_flag) {
			for (int i = 0; i < way; ++i) {
				L1_data[i][set].LRU_bits = --L1_data[i][set].LRU_bits; //pre-decrement the LRU bits
			}
		}
		// If a way is NOT empty, we compare LRU bits of the current set with the LRU bits of the other sets
		else {
			for (int i = 0; i < 8; ++i) {
				if (L1_data[way][set].LRU_bits > L1_data[i][set].LRU_bits) {
					L1_data[i][set].LRU_bits = L1_data[i][set].LRU_bits;    //no need to do anything, keep the same order
				}
				else
				{
					L1_data[i][set].LRU_bits = --L1_data[i][set].LRU_bits; //pre-decrement the LRU bits
				}
			}
		}
		L1_data[way][set].LRU_bits = 0x7;	// Set the current set to MRU 111 (0x7 in Hex)
		break;



	case ('i'):
	case ('I'): 	// We're in the L1_instruction cache if which_chache is 'I' or 'i'
		// If a way is empty, we use it and decrement every way less than this way
		if (empty_flag) {
			for (int i = 0; i < way; ++i) {
				L1_inst[i][set].LRU_bits = --L1_inst[i][set].LRU_bits;
			}
		}
		// If a way is NOT empty, we compare LRU bits of the current set with the LRU bits of the other sets
		else {
			for (int i = 0; i < 4; ++i) {
				if (L1_inst[way][set].LRU_bits > L1_inst[i][set].LRU_bits) {
					L1_inst[i][set].LRU_bits = L1_inst[i][set].LRU_bits;      //no need to do anything, keep the same order
				}
				else
				{
					--L1_inst[i][set].LRU_bits;                  //pre-decrement the LRU bits
				}
			}
		}
		L1_inst[way][set].LRU_bits = 0x3; // Set the current set to MRU 11 (0x3 in Hex)

		break;
	}
}


/////////////////////////////////////////////////////////////

/* Find the Least Recently Used Way function
*  This funcion finds the way which includes this set and has an LRU of 0
*  This function takes in he set, and a which_cache flag
*  If which cache is 'D' or 'd' we want to find the Data cache LRU
*  If which cache is 'I' or 'i' we want to find the instruction cache LRU
*  The function returns the way least recently used (to be evicted). 
*  If we can't find a LRU way, something is wrong, and we return -1
*/
int find_LRU(unsigned int set, char which_cache) {
	switch (which_cache) {
	case ('d'):
	case ('D'): 	// We're in the L1_data cache if which_chache is 'D' or 'd'
		for (int i = 0; i < 8; ++i) {
			if (L1_data[i][set].LRU_bits == 0) {
				return i;
			}
		}
		break;

	case ('i'):
	case ('I'): 	// We're in the L1_instruction cache if which_chache is 'I' or 'i'

		for (int i = 0; i < 4; ++i) {
			if (L1_inst[i][set].LRU_bits == 0) {
				return i;
			}
		}
		break;
	}
	return -1; // This means that something is wrong! We expected an LRU (0) but we couldn't find any
	}
