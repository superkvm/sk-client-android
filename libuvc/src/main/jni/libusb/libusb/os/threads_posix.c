

#if defined(__linux__) || defined(__OpenBSD__)
# if defined(__linux__)
#  define _GNU_SOURCE
# else
#  define _BSD_SOURCE
# endif
# include <unistd.h>
# include <sys/syscall.h>
#elif defined(__APPLE__)
# include <mach/mach.h>
#elif defined(__CYGWIN__)
# include <windows.h>
#endif

#include "threads_posix.h"

int usbi_mutex_init_recursive(pthread_mutex_t *mutex, pthread_mutexattr_t *attr) {
	int err;
	pthread_mutexattr_t stack_attr;
	if (!attr) {
		attr = &stack_attr;
		err = pthread_mutexattr_init(&stack_attr);
		if (err != 0)
			return err;
	}

	
	err = pthread_mutexattr_settype(attr, PTHREAD_MUTEX_RECURSIVE);
	if (err != 0)
		goto finish;

	err = pthread_mutex_init(mutex, attr);

finish:
	if (attr == &stack_attr)
		pthread_mutexattr_destroy(&stack_attr);

	return err;
}

int usbi_get_tid(void) {
	int ret = -1;
#if defined(__ANDROID__)
	ret = gettid();
#elif defined(__linux__)
	ret = syscall(SYS_gettid);
#elif defined(__OpenBSD__)
	
	ret = syscall(SYS_getthrid);
#elif defined(__APPLE__)
	ret = mach_thread_self();
	mach_port_deallocate(mach_task_self(), ret);
#elif defined(__CYGWIN__)
	ret = GetCurrentThreadId();
#endif
	
	return ret;
}
