
#if 1

#define NTOKENS  			2063592L
#define MAX22   			4194304L

#define MAXGRID4  			32400

//#pragma once
//#include <stdint.h>

//namespace ft8_v2 {

// Pack FT8 text message into 72 bits
// [IN] msg      - FT8 message (e.g. "CQ TE5T KN01")
// [OUT] c77     - 10 byte array to store the 77 bit payload (MSB first)
int pack77(const char *msg, uint8_t *c77);

//};

#endif
