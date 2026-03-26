

#ifdef _WIN32

#include <windows.h>

static double getfreq(void)
{
	LARGE_INTEGER freq;
	if(!QueryPerformanceFrequency(&freq)) return 0.0;
	return (double)freq.QuadPart;
}

static double f=-1.0;

double gettime(void)
{
	LARGE_INTEGER t;
	if(f<0.0) f=getfreq();
	if(f==0.0) return (double)GetTickCount()/1000.;
	else
	{
		QueryPerformanceCounter(&t);
		return (double)t.QuadPart/f;
	}
}

#else

#include <stdlib.h>
#include <sys/time.h>

double gettime(void)
{
	struct timeval tv;
	if(gettimeofday(&tv, NULL)<0) return 0.0;
	else return (double)tv.tv_sec+((double)tv.tv_usec/1000000.);
}

#endif
