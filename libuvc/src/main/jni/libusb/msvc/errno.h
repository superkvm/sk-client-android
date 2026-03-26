

#ifndef _ERRNO_H_
#define	_ERRNO_H_

#include <crtdefs.h>


#define EPERM		1	
#define	ENOFILE		2	
#define	ENOENT		2
#define	ESRCH		3	
#define	EINTR		4	
#define	EIO		5	
#define	ENXIO		6	
#define	E2BIG		7	
#define	ENOEXEC		8	
#define	EBADF		9	
#define	ECHILD		10	
#define	EAGAIN		11	
#define	ENOMEM		12	
#define	EACCES		13	
#define	EFAULT		14	

#define	EBUSY		16	
#define	EEXIST		17	
#define	EXDEV		18	
#define	ENODEV		19	
#define	ENOTDIR		20	
#define	EISDIR		21	
#define	EINVAL		22	
#define	ENFILE		23	
#define	EMFILE		24	
#define	ENOTTY		25	

#define	EFBIG		27	
#define	ENOSPC		28	
#define	ESPIPE		29	
#define	EROFS		30	
#define	EMLINK		31	
#define	EPIPE		32	
#define	EDOM		33	
#define	ERANGE		34	

#define	EDEADLOCK	36	
#define	EDEADLK		36
#if 0

#define	ENAMETOOLONG	38	
#define	ENOLCK		39	
#define	ENOSYS		40	
#define	ENOTEMPTY	41	
#define	EILSEQ		42	
#endif



#ifndef	RC_INVOKED

#ifdef	__cplusplus
extern "C" {
#endif


#if defined(_UWIN) || defined(_WIN32_WCE)
#undef errno
extern int errno;
#else
_CRTIMP int* __cdecl _errno(void);
#define	errno		(*_errno())
#endif

#ifdef	__cplusplus
}
#endif

#endif	

#endif	