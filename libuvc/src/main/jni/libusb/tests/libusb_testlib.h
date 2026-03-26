

#ifndef LIBUSB_TESTLIB_H
#define LIBUSB_TESTLIB_H

#include <stdio.h>

#if !defined(bool)
#define bool int
#endif
#if !defined(true)
#define true (1 == 1)
#endif
#if !defined(false)
#define false (!true)
#endif


typedef enum {
	
	TEST_STATUS_SUCCESS,
	
	TEST_STATUS_FAILURE,
	
	TEST_STATUS_ERROR,
	
	TEST_STATUS_SKIP
} libusb_testlib_result;


typedef struct {
	char ** test_names;
	int test_count;
	bool list_tests;
	bool verbose;
	int old_stdout;
	int old_stderr;
	FILE* output_file;
	int null_fd;
} libusb_testlib_ctx;


void libusb_testlib_logf(libusb_testlib_ctx * ctx, 
                          const char* fmt, ...);


typedef libusb_testlib_result
(*libusb_testlib_test_function)(libusb_testlib_ctx * ctx);


typedef struct {
	
	const char * name;
	
	libusb_testlib_test_function function;
} libusb_testlib_test;


#define LIBUSB_NULL_TEST {NULL, NULL}


int libusb_testlib_run_tests(int argc,
                              char ** argv,
                              const libusb_testlib_test * tests);

#endif 
