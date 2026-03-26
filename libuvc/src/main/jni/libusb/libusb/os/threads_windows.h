

#ifndef LIBUSB_THREADS_WINDOWS_H
#define LIBUSB_THREADS_WINDOWS_H

#define usbi_mutex_static_t     volatile LONG
#define USBI_MUTEX_INITIALIZER  0

#define usbi_mutex_t            HANDLE

struct usbi_cond_perthread {
	struct list_head list;
	DWORD            tid;
	HANDLE           event;
};
struct usbi_cond_t_ {
	
	
	
	struct list_head waiters;
	struct list_head not_waiting;
};
typedef struct usbi_cond_t_ usbi_cond_t;


#if (!defined(HAVE_STRUCT_TIMESPEC) && !defined(_TIMESPEC_DEFINED))
#define HAVE_STRUCT_TIMESPEC 1
#define _TIMESPEC_DEFINED 1
struct timespec {
		long tv_sec;
		long tv_nsec;
};
#endif 


#ifndef ETIMEDOUT
#  define ETIMEDOUT 10060     
#endif

#define usbi_mutexattr_t void
#define usbi_condattr_t  void


#define usbi_mutex_init_recursive(mutex, attr) usbi_mutex_init((mutex), (attr))

int usbi_mutex_static_lock(usbi_mutex_static_t *mutex);
int usbi_mutex_static_unlock(usbi_mutex_static_t *mutex);


int usbi_mutex_init(usbi_mutex_t *mutex,
					const usbi_mutexattr_t *attr);
int usbi_mutex_lock(usbi_mutex_t *mutex);
int usbi_mutex_unlock(usbi_mutex_t *mutex);
int usbi_mutex_trylock(usbi_mutex_t *mutex);
int usbi_mutex_destroy(usbi_mutex_t *mutex);

int usbi_cond_init(usbi_cond_t *cond,
				   const usbi_condattr_t *attr);
int usbi_cond_destroy(usbi_cond_t *cond);
int usbi_cond_wait(usbi_cond_t *cond, usbi_mutex_t *mutex);
int usbi_cond_timedwait(usbi_cond_t *cond,
						usbi_mutex_t *mutex,
						const struct timespec *abstime);
int usbi_cond_broadcast(usbi_cond_t *cond);
int usbi_cond_signal(usbi_cond_t *cond);

int usbi_get_tid(void);

#endif 
