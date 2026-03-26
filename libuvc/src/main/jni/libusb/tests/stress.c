

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "libusb.h"
#include "libusb_testlib.h"


static libusb_testlib_result test_init_and_exit(libusb_testlib_ctx * tctx)
{
	libusb_context * ctx = NULL;
	int i;
	for (i = 0; i < 10000; ++i) {
		int r = libusb_init(&ctx);
		if (r != LIBUSB_SUCCESS) {
			libusb_testlib_logf(tctx,
				"Failed to init libusb on iteration %d: %d",
				i, r);
			return TEST_STATUS_FAILURE;
		}
		libusb_exit(ctx);
		ctx = NULL;
	}

	return TEST_STATUS_SUCCESS;
}


static libusb_testlib_result test_get_device_list(libusb_testlib_ctx * tctx)
{
	libusb_context * ctx = NULL;
	int r, i;
	r = libusb_init(&ctx);
	if (r != LIBUSB_SUCCESS) {
		libusb_testlib_logf(tctx, "Failed to init libusb: %d", r);
		return TEST_STATUS_FAILURE;
	}
	for (i = 0; i < 1000; ++i) {
		libusb_device ** device_list;
		ssize_t list_size = libusb_get_device_list(ctx, &device_list);
		if (list_size < 0 || device_list == NULL) {
			libusb_testlib_logf(tctx,
				"Failed to get device list on iteration %d: %d (%p)",
				i, -list_size, device_list);
			return TEST_STATUS_FAILURE;
		}
		libusb_free_device_list(device_list, 1);
	}
	libusb_exit(ctx);
	return TEST_STATUS_SUCCESS;
}


static libusb_testlib_result test_many_device_lists(libusb_testlib_ctx * tctx)
{
#define LIST_COUNT 100
	libusb_context * ctx = NULL;
	libusb_device ** device_lists[LIST_COUNT];
	int r, i;
	memset(device_lists, 0, sizeof(device_lists));

	r = libusb_init(&ctx);
	if (r != LIBUSB_SUCCESS) {
		libusb_testlib_logf(tctx, "Failed to init libusb: %d", r);
		return TEST_STATUS_FAILURE;
	}

	
	for (i = 0; i < LIST_COUNT; ++i) {
		ssize_t list_size = libusb_get_device_list(ctx, &(device_lists[i]));
		if (list_size < 0 || device_lists[i] == NULL) {
			libusb_testlib_logf(tctx,
				"Failed to get device list on iteration %d: %d (%p)",
				i, -list_size, device_lists[i]);
			return TEST_STATUS_FAILURE;
		}
	}

	
	for (i = 0; i < LIST_COUNT; ++i) {
		if (device_lists[i]) {
			libusb_free_device_list(device_lists[i], 1);
			device_lists[i] = NULL;
		}
	}

	libusb_exit(ctx);
	return TEST_STATUS_SUCCESS;
#undef LIST_COUNT
}


static libusb_testlib_result test_default_context_change(libusb_testlib_ctx * tctx)
{
	libusb_context * ctx = NULL;
	int r, i;

	for (i = 0; i < 100; ++i) {
		
		r = libusb_init(&ctx);
		if (r != LIBUSB_SUCCESS) {
			libusb_testlib_logf(tctx, "Failed to init libusb: %d", r);
			return TEST_STATUS_FAILURE;
		}

		
		libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
		libusb_set_debug(ctx, LIBUSB_LOG_LEVEL_DEBUG);

		
		r = libusb_init(NULL);
		if (r != LIBUSB_SUCCESS) {
			libusb_testlib_logf(tctx, "Failed to init libusb: %d", r);
			return TEST_STATUS_FAILURE;
		}

		
		libusb_exit(ctx);
		
		libusb_exit(NULL);
	}

	return TEST_STATUS_SUCCESS;
}


static const libusb_testlib_test tests[] = {
	{"init_and_exit", &test_init_and_exit},
	{"get_device_list", &test_get_device_list},
	{"many_device_lists", &test_many_device_lists},
	{"default_context_change", &test_default_context_change},
	LIBUSB_NULL_TEST
};

int main (int argc, char ** argv)
{
	return libusb_testlib_run_tests(argc, argv, tests);
}
