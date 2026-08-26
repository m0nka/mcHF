// integer.h shim — ff16's diskio.h includes this for base types.
// ff.h defines them all, so just pull them from stdint.h here.

#ifndef _FF_INTEGER
#define _FF_INTEGER

#include <stdint.h>

typedef unsigned int	UINT;
typedef unsigned char	BYTE;
typedef uint16_t		WORD;
typedef unsigned short	WCHAR;
typedef uint32_t		DWORD;
typedef uint64_t		QWORD;

#endif
