#ifndef __UNPACK_V2_H
#define __UNPACK_V2_H

//#pragma once
//#include <stdint.h>

// message should have at least 19 bytes allocated (18 characters + zero terminator)
int unpack77(const uint8_t *a77, char *message);

#endif
