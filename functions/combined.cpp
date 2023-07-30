#include "header.h"

/*
	ECE 585 Microprocessor System Design Final Project
	Winter 2023 - Team 1
	Mohamed Ghonim - Ahliah Nordstrom - Brandon Bieschke - Celina Wong - Assefa Setgen

*/

/* This is the main function of our project
*   It promots the user to enter the mode (0 or 1) of operation,
*	takes in the file name of the trace txt file
*	and  calls the import function
*/
int main(int argc, char** argv) {

	// Initialize the cache at the beginning
	clear_cache();

	cout << "\n  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" << endl;
	cout << "\t Simulation of an L1 split cache of a 32-bit Processor\n" << endl;
	cout << "\t      ECE 585 Final Project Winter 2023 - Team 1" << endl;
	cout << "\n  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n" << endl;
	cout << "  Mode 0: Display only the required summary of usage statistics and responses to 9s in the trace file." << endl;
	cout << "  Mode 1: Display everything from mode 1, as well as the communication messages to the L2 cache." << endl;

	do {
		cout << "\n  Please enter the mode number you'd like to select (0,1): ", cin >> mode;
		if (mode > 1) {
			cout << "\n\tInvalid mode value." << endl;
		}
	} while (mode > 1);

	cout << "\n";	//spacing to make it look neater

	// call the parser function, and give it the command line file names
	parser(argc,argv);

	cout << "\n\t --- L1 Split Cache Anaylsis and Simulation Completed! --- " << endl;
	return 0;
}

/* This is the file parser function
*  It reads the trace file and gets the operation and the address from each line in the file
*  The function doesn't return anything, but it takes the program to the respective n functions
*  Depending on the operation n */
void parser(int argc, char** argv) {

	// Reading the trace file from the command line
	char* trace_file = argv[1];		//argv is an array of the strings

	// Check for command line test file
	if (argc != 2) {		// if the count of arguments is not 2
		cout << "\n Not able to read the trace file!" << endl;
		exit(1);			// exit (0) is exit after successfull run, exit(1) exit after failure
	}
		
	char trace_line[1024];								// the address is 8 char (hex) and one black space (1) and one op char = 10 char = 2^10 = 1024 	
	char trace_operation[1];							// n is one character long

	unsigned int operation;								// Operation parsed from input
	unsigned int address;								// Address parsed from input
	FILE* fp;											// .txt test file pointer (fp)

	fp = fopen(trace_file, "r");						// "r" switch of the fopen function opens a file for reading

	while (fgets(trace_line, 1024 , fp)) {

		// sscanf reads the data from trace_line and stores it in trace_operation and address
		sscanf(trace_line, "%c %x", trace_operation, &address);	// %c is a swtich for a single character, %x is for hexacdecimal in lowercase letters
		if (!strcmp(trace_line, "\n") || !strcmp(trace_line, " ")) { //strcmp compares two strings 
		}
		else {
			operation = atoi(trace_operation);		//Parses the C-string str interpreting its content as an integral number, which is returned as a value of type int.
			switch (operation) {
			case 0:	read(address);			break;
			case 1:	write(address);			break;
			case 2:	fetch_inst(address);	break;
			case 3:	invalidate(address);	break;	
			case 4:	snoop(address);			break;
			case 8:	clear_cache();			break;
			case 9:	print_stats();			break;
			default:
				cout << "\n the value of n (the operation) is not valid \n" << endl;
				break;
			}
		}
	}
	fclose(fp);
	return;
}

/*************
****
****
* Functions
****************************/
/* Data read Function
 * in response to n = 0 in the trace file
 * This function attempts to read a an address/line from the data cache
 * If we have a miss, we place the line in an empty line
 * if no empty lines, we check for an a line in the invalid state, evict it and place the line there
 * if not invalid states, we evict the LRU and place the line in that way
 * This function takes in an address, and doesn't need to return anything
 * For the data read MESI protocol we have:
 * If we're in 'E' we go to 'S', if we're in 'S' stay in 'S', if in 'M' stay in 'M', if in 'I' go to 'E'
 */
void read(unsigned int addr) {

	unsigned int tag = addr >> (BYTE_OFFSET + CACHE_INDEX);				// Shift the address right by (6+14=20) to get the tag
	unsigned int set = (addr & MASK_CACHE_INDEX) >> BYTE_OFFSET;		// Mask the address with 0x000FFFFF and shift by 6 to get the cache index/set
	bool empty_flag = 0;												// boolean flag for empty way in the cache, if 1, we have an empty way
	int way = -1;														// way in the cache set. Initialized with an invalid way value
	statistics.data_read++;												// Increment the number of reads for the data cache

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

		case 'E':									// if we're in the exclusive state, assume a different processor is reading
			statistics.data_hit++;					// Incremenet the data hit counter (We have a new hit!)
			L1_data[way][set].MESI_char = 'S';		// Move the Shared state, another processor is doing the read operation
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');		// Update the data cache LRU count
			break;

		case 'S':									// if we are in the shared state
			statistics.data_hit++;					// Incremenet the data hit counter (We have a new hit!)
			L1_data[way][set].MESI_char = 'S';		// remain in the shared state
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');		// Update the data cache LRU count
			break;

		case 'I':									// if we are in the invalid state, we'll have a miss
			statistics.data_miss++;					// Incremenet the data miss counter 
			L1_data[way][set].MESI_char = 'S';		// and we move to the Shared state. 
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');		// Update the data cache LRU count

			if (mode == 1) {						// Simulating a cache read communication with L2
				cout << " [Data-Operation] Read from L2: L1 cache miss, obtain data form L2 " << hex << addr << endl;
			}
			break;
		}
	}


	else {							// if we don't have a matching tag, we have a data cache miss
		statistics.data_miss++;							// Increment the data cache miss counter

		for (int i = 0; (way < 0 && i < 8); ++i) {		// First, check if we have any empty lines
			if (L1_data[i][set].tag_bits == EMPTY_TAG) {
				way = i;								// return the way that has an empty line
				empty_flag = 1;							// if we have an empty line, toggle the empty_flag to high
			}
		}

		if (way >= 0) {									// if we have an empty line, place the read data in it
			L1_data[way][set].MESI_char = 'E';			// and mark it exclusive, it's the only line with this data
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');			// Update the data cache LRU order/count

			if (mode == 1) {							// Simulating a cache read communication with L2
				cout << " [Data-Operation] Read from L2: L1 cache miss, obtain data form L2 " << hex << addr << endl;
			}
		}

		else {					// if we don't have any empty lines, we need to evict the LRU line. 
			if (mode == 1) {
				cout << " [Data-Operation] Read from L2: L1 cache miss, obtain data from L2 " << hex << addr << endl;
			}

			for (int n = 0; n < 8; ++n) {
				if (L1_data[n][set].MESI_char == 'I') {	// Since we don't have empty lines, we check for a way with an invalid lines to evict
					way = n;
				}
				else {
					way = -1;				// if we don't have any invalid data cache lines, flag the  way with "-1"
				}
			}

			if (way < 0) {					//If we don't have any invalid lines, 
				way = find_LRU(set, 'D');				// Find the LRU way in the data cache
				if (way >= 0) {							// if we have a way that's 0, (LRU in the data cache, we use it)
					L1_data[way][set].MESI_char = 'E';	// the data here will be new, and exclusive to this cache
					L1_data[way][set].tag_bits = tag;
					L1_data[way][set].set_bits = set;
					L1_data[way][set].address = addr;

					L1_LRU(way, set, empty_flag, 'D');	// update the L1 data cache LRU bits
				}
			}

			else { 							// if we have an invalid way, we evict it and 
				L1_data[way][set].MESI_char = 'E';		// the data here will be new, and exclusive to this cache
				L1_data[way][set].tag_bits = tag;
				L1_data[way][set].set_bits = set;
				L1_data[way][set].address = addr;

				L1_LRU(way, set, empty_flag, 'D');
			}
		}
	}
	return;
}

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
	statistics.data_write++;											// Increment the number of write for the data cache

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

	else {											// Data Cache Miss
		statistics.data_miss++;						// Incremenet the data miss counter 

		if (mode == 1) {							// Simulating a cache Read For Ownership communication from L2
			cout << " [Data-Operation] Read for Ownership from L2 " << hex << addr << endl;
		}

		for (int i = 0; way < 0 && i < 8; ++i) {		// First, check if we have any empty lines
			if (L1_data[i][set].tag_bits == EMPTY_TAG) {
				way = i;								// return the way that has an empty line
				empty_flag = 1;							// if we have an empty line, toggle the empty_flag to high
			}
		}

		if (way >= 0) {									// if we have an empty line, place the read data in it
			L1_data[way][set].MESI_char = 'M';			// go to the modified state
			L1_data[way][set].tag_bits = tag;
			L1_data[way][set].set_bits = set;
			L1_data[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'D');			// Update the data cache LRU order/count

			if (mode == 1) {							// Simulating an iniial write through communication from L2
				cout << " [Data Write_Through] Write to L2: we have a data Cache Miss " << hex << addr << endl;
			}
		}

		else {											// if we don't have any empty lines, we need to evict the LRU line. 
			if (mode == 1) {							// Simulating an write back communication from L2
				cout << " [Data Write_Back] Write to L2: We have a data Cache Miss " << hex << addr << endl;
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
	way = matching_tag(tag, set, 'I');					// Look a matching tag already in the instruction cache

	if (way >= 0) {										// if we have a matching tag, then we have an instruction cache hit! (unless invalid MESI state)
		switch (L1_inst[way][set].MESI_char) {			// FSM implementing the MESI protocol
		case 'M':										// If we are in the modified state
			statistics.inst_hit++;						// We have a valid hit, increment the instruction cache hit counter
			L1_inst[way][set].MESI_char = 'M';			// We remain in the modified MESI state
			L1_inst[way][set].tag_bits = tag;
			L1_inst[way][set].set_bits = set;
			L1_inst[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'I');			// Update the instruction cache LRU count
			break;

		case 'E':										// If we are in the modified state, another processor is fetching
			statistics.inst_hit++;						// We have a valid hit, increment the instruction cache hit counter
			L1_inst[way][set].MESI_char = 'S';			// Move the Shared state
			L1_inst[way][set].tag_bits = tag;
			L1_inst[way][set].set_bits = set;
			L1_inst[way][set].address = addr;

			L1_LRU(way, set, empty_flag, 'I');			// Update the instruction cache LRU count
			break;

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
				cout << " [Instruction-Operation] Read from L2: L1 cache miss, obtain instruction form L2 " << hex << addr << endl;
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
				cout << " [Instruction-Operation] Read from L2: L1 cache miss, obtain instruction form L2 " << hex << addr << endl;
			}
		}

		else {						// if we don't have any empty lines, we need to evict the LRU line. 
			if (mode == 1) {
				cout << " [Instruction-Operation] Read from L2: L1 cache miss, obtain intsruction from L2 " << hex << endl;
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
				way = find_LRU(set, 'I');				// Find the LRU way in the instruction cache
				if (way >= 0) {							// if we have a way that's 0, (LRU in the instruction cache, we use it)
					L1_inst[way][set].MESI_char = 'E';	// the instruction here will be new, and exclusive to this cache
					L1_inst[way][set].tag_bits = tag;
					L1_inst[way][set].set_bits = set;
					L1_inst[way][set].address = addr;

					L1_LRU(way, set, empty_flag, 'I');	// update the L1 instruction cache LRU bits
				}
			}
			else { 					// if we have an invalid way, we evict it and 
				L1_inst[way][set].MESI_char = 'E';		// the instruction here will be new, and exclusive to this cache
				L1_inst[way][set].tag_bits = tag;
				L1_inst[way][set].set_bits = set;
				L1_inst[way][set].address = addr;

				L1_LRU(way, set, empty_flag, 'I');
			}
		}
	}
	return;
}

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
			L1_data[i][set].MESI_char = 'I';			// Change MESI bit to Invalid
			L1_data[i][set].tag_bits = tag;
			L1_data[i][set].set_bits = set;
			L1_data[i][set].address = addr;

			if (mode == 1) {							// Simulating a snoop return data communication with L2
				cout << "[Snoop-operation] Return data to L2 " << hex << addr << endl;
			}
		}
	}
	return;
}

/*
*	When this function is called it clears the cache to reset it to the intitial empty state
*	and clears all the statisticsitics.
 */
void clear_cache() {

	// cout << "\n\t Clear the cache to the initial state and reset the statistics" << endl;
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
void L1_LRU(unsigned int way, unsigned int set, bool empty_flag, char which_cache) {
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
			if (L1_data[i][set].LRU_bits == 0) {			// find the set with the LRU bits of 0 (Least Recently Used)
				return i;									// return the way/index in which the LRU is
			}
		}
		break;

	case ('i'):
	case ('I'): 	// We're in the L1_instruction cache if which_chache is 'I' or 'i'

		for (int i = 0; i < 4; ++i) {
			if (L1_inst[i][set].LRU_bits == 0) {			// find the set with the LRU bits of 0 (Least Recently Used)
				return i;									// return the way/index in which the LRU is
			}
		}
		break;
	}
	return -1; // This means that something is wrong! We expected an LRU (0) but we couldn't find any
}

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
	switch (which_cache) {
	case ('d'):
	case ('D'):							// if flag is 'D' We're searching for a matching tag in the L1 data cache
		while (L1_data[i][set].tag_bits != tag) {		// search the data cache for matching tags
			i++;
			if (i > 7) {				// We have 8 ways in the data cache 0 through 7
				return -1;				// return -1 to imply that we don't have a matching tag in the data cache. 
			}
		}
		return i;
		break;

	case ('i'):
	case ('I'):						// if flag is 'I' We're searching for a matching tag in the L1 instruction cache
		int i = 0;
		while (L1_inst[i][set].tag_bits != tag) {			// search the instruction cache for matching tags
			i++;
			if (i > 3) {			// We have 4 ways in the instruction cache 0 through 3
				return -1;			// return -1 to imply that we don't have a matching tag in the instruction cache.
			}
		}
	}
	return i;						// If we have a matching tag, return the way in which we have the matching tag. f
}

/* Print cache content and state
 * in response to n = 9 in the trace file
 * This function doesn't take any inputs, and doesn't return any outputs
 * when called, it outputs the number of:
 *		cache reads, writes, hits, misses, hit ratio for the data cache, and
 *		cache reads/fetches, hits, misses, hit ratio for the instruction cache
 */
void print_stats() {

	//Calculate the hit ratio for the data and the instrucion caches
	float data_hit_r = float(statistics.data_hit) / (float(statistics.data_miss) + float(statistics.data_hit));
	float inst_hit_r = float(statistics.inst_hit) / (float(statistics.inst_miss) + float(statistics.inst_hit));

	statistics.data_hit_ratio = data_hit_r;
	statistics.inst_hit_ratio = inst_hit_r;

	cout << "\n \t ** KEY CACHE USAGE STATISTICS ** " << endl;
	if (statistics.data_miss == 0) {				// If we don't have any misses, then no operations took place!
		cout << "no operations took place on the Data cache" << endl;
	}
	else {
		cout << "\n\t -- DATA CACHE STATISTICS -- " << endl;

		cout << " number of Cache Reads: \t" << dec << statistics.data_read << endl;
		cout << " number of Cache Writes: \t" << dec << statistics.data_write << endl;
		cout << " number of Cache Hits: \t \t" << dec << statistics.data_hit << endl;
		cout << " number of Cache Misses: \t" << dec << statistics.data_miss << endl;
		cout << " Cache Hit Ratio: \t\t" << dec << statistics.data_hit_ratio << endl;
		cout << " Cache Hit Percentage: \t" << dec << (statistics.data_hit_ratio * 100) << " %" << endl;
	}

	if (statistics.inst_miss == 0) {				// If we don't have any misses, then no operations took place!
		cout << "\n The cache instruction was not used/not operated on" << endl;
	}
	else {
		cout << "\n\t -- INSTRUCTION CACHE STATISTICS -- " << endl;

		cout << " number of Cache Reads: \t" << dec << statistics.inst_read << endl;
		cout << " number of Cache Hits: \t\t" << dec << statistics.inst_hit << endl;
		cout << " number of Cache Misses: \t" << dec << statistics.inst_miss << endl;
		cout << " Cache Hit Ratio: \t\t" << dec << statistics.inst_hit_ratio << endl;
		cout << " Cache Hit Percentage: \t" << dec << (statistics.inst_hit_ratio) * 100 << " %" << endl;
	}
	return;
}
