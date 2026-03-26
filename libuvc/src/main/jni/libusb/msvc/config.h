

#ifndef _MSC_VER
#warn "msvc/config.h shouldn't be included for your development environment."
#error "Please make sure the msvc/ directory is removed from your build path."
#endif


#pragma warning(disable:4200)

#pragma warning(disable: 6258)
#if defined(_PREFAST_)

#pragma warning(disable:28719)

#pragma warning(disable:28125)
#endif


#define DEFAULT_VISIBILITY 


#define ENABLE_LOGGING 1








#define POLL_NFDS_TYPE unsigned int


#if defined(_WIN32_WCE)
#define OS_WINCE 1
#define HAVE_MISSING_H
#else
#define OS_WINDOWS 1
#define HAVE_SIGNAL_H 1
#define HAVE_SYS_TYPES_H 1
#endif
