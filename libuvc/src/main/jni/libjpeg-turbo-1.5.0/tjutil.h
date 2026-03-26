

#ifdef _WIN32
	#ifndef __MINGW32__
		#include <stdio.h>
		#define snprintf(str, n, format, ...)  \
			_snprintf_s(str, n, _TRUNCATE, format, __VA_ARGS__)
	#endif
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
#endif

#ifndef min
 #define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
 #define max(a,b) ((a)>(b)?(a):(b))
#endif

extern double gettime(void);
