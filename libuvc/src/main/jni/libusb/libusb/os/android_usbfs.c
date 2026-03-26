



#define LOCAL_DEBUG 0

#define LOG_TAG "libusb/usbfs"
#if 1	
	#ifndef LOG_NDEBUG
		#define	LOG_NDEBUG		
		#endif
	#undef USE_LOGALL			
#else
	#define USE_LOGALL
	#undef LOG_NDEBUG
	#undef NDEBUG
	#define GET_RAW_DESCRIPTOR
#endif

#include "config.h"
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "libusb.h"
#include "libusbi.h"
#include "android_usbfs.h"





static const char *usbfs_path = NULL;


static int usbdev_names = 0;


static int supports_flag_bulk_continuation = -1;


static int supports_flag_zero_packet = -1;


static clockid_t monotonic_clkid = -1;


static int sysfs_can_relate_devices = -1;


static int sysfs_has_descriptors = -1;


static int init_count = 0;


usbi_mutex_static_t android_hotplug_startstop_lock = USBI_MUTEX_INITIALIZER;

usbi_mutex_static_t android_hotplug_lock = USBI_MUTEX_INITIALIZER;

static int android_start_event_monitor(void);
static int android_stop_event_monitor(void);
static int android_scan_devices(struct libusb_context *ctx);
static int sysfs_scan_device(struct libusb_context *ctx, const char *devname);
static int detach_kernel_driver_and_claim(struct libusb_device_handle *, int);

#if !defined(USE_UDEV)
static int android_default_scan_devices(struct libusb_context *ctx);
#endif

struct android_device_priv {
	char *sysfs_dir;
	unsigned char *descriptors;
	int descriptors_len;
	int active_config; 
	int fd;
};

struct android_device_handle_priv {
	int fd;
	uint32_t caps;
};

enum reap_action {
	NORMAL = 0,
	
	SUBMIT_FAILED,

	
	CANCELLED,

	
	COMPLETED_EARLY,

	
	ERROR,
};

struct android_transfer_priv {
	union {
		struct usbfs_urb *urbs;
		struct usbfs_urb **iso_urbs;
	};

	enum reap_action reap_action;
	int num_urbs;
	int num_retired;
	enum libusb_transfer_status reap_status;

	
	int iso_packet_offset;
};

#if LOCAL_DEBUG
static void dump_urb(int ix, int fd, struct usbfs_urb *urb) {
	LOGI("%d:fd=%d", ix, fd);
	int ret = fcntl(fd, F_GETFL);
	if (UNLIKELY(ret == -1)) {
		LOGE("Failed to get fd flags: %d", errno);
	}
	LOGI("ファイフディスクリプタフラグ:%x", ret);
	LOGI("O_ACCMODE:%x", ret & O_ACCMODE);				
	LOGI("ノンブロッキングかどうか:%d", ret & O_NONBLOCK);	
	LOGI("%d:type=%d,endpopint=0x%02x,status=%d,flag=%d", ix, urb->type, urb->endpoint, urb->status, urb->flags);
	LOGI("%d:buffer=%p,buffer_length=%d,actual_length=%d,start_frame=%d", ix, urb->buffer, urb->buffer_length, urb->actual_length, urb->start_frame);
	LOGI("%d:number_of_packets=%d,error_count=%d,signr=%d", ix, urb->number_of_packets, urb->error_count, urb->signr);
	LOGI("%d:usercontext=%p,iso_frame_desc=%p", ix, urb->usercontext, urb->iso_frame_desc);
}
#endif


static int __get_usbfs_fd(struct libusb_device *dev, mode_t mode, int silent) {

	struct libusb_context *ctx = DEVICE_CTX(dev);
	char path[PATH_MAX];
	int fd;
	int delay = 10000;

	if (usbdev_names)
		snprintf(path, PATH_MAX, "%s/usbdev%d.%d",
			usbfs_path, dev->bus_number, dev->device_address);
	else
		snprintf(path, PATH_MAX, "%s/%03d/%03d",
			usbfs_path, dev->bus_number, dev->device_address);

	fd = open(path, mode);
	if (LIKELY(fd != -1))
		return fd; 

	if (errno == ENOENT) {
		if (!silent)
			usbi_err(ctx, "File doesn't exist, wait %d ms and try again\n", delay / 1000);

		
		usleep(delay);

		fd = open(path, mode);
		if (LIKELY(fd != -1))
			return fd; 
	}

	if (!silent) {
		usbi_err(ctx, "libusb couldn't open USB device %s: %s",
			path, strerror(errno));
		if (errno == EACCES && mode == O_RDWR)
			usbi_err(ctx, "libusb requires write access to USB "
					"device nodes.");
	}

	if (errno == EACCES)
		return LIBUSB_ERROR_ACCESS;
	if (errno == ENOENT)
		return LIBUSB_ERROR_NO_DEVICE;
	return LIBUSB_ERROR_IO;
}

static struct android_device_priv *_device_priv(struct libusb_device *device);
static int _get_usbfs_fd(struct libusb_device *device, mode_t mode, int silent) {
#ifdef __ANDROID__
	struct android_device_priv *dpriv = _device_priv(device);

	if (LIKELY(dpriv->fd > 0))
		return dpriv->fd;
	else {
		
		
#if !defined(__LP64__)
		usbi_dbg("fd have not set yet. device=%x,fd=%d", (int )device, dpriv->fd);
#else
		usbi_dbg("fd have not set yet. device=%x,fd=%d", (long )device, dpriv->fd);
#endif
		return __get_usbfs_fd(device, mode, silent);
	}
#else
	return __get_usbfs_fd(device, mode, silent);
#endif
}

static struct android_device_priv *_device_priv(struct libusb_device *dev) {
	return (struct android_device_priv *) dev->os_priv;
}

static struct android_device_handle_priv *_device_handle_priv(
		struct libusb_device_handle *handle) {
	return (struct android_device_handle_priv *) handle->os_priv;
}


static int _is_usbdev_entry(struct dirent *entry, int *bus_p, int *dev_p) {
	int busnum, devnum;

	if (sscanf(entry->d_name, "usbdev%d.%d", &busnum, &devnum) != 2)
		return LIBUSB_SUCCESS;

	usbi_dbg("found: %s", entry->d_name);
	if (bus_p != NULL)
		*bus_p = busnum;
	if (dev_p != NULL)
		*dev_p = devnum;
	return 1;
}

static int check_usb_vfs(const char *dirname) {
	DIR *dir;
	struct dirent *entry;
	int found = 0;

	dir = opendir(dirname);
	if (!dir)
		return LIBUSB_SUCCESS;

	while ((entry = readdir(dir)) != NULL ) {
		if (entry->d_name[0] == '.')
			continue;

		
		found = 1;
		break;
	}

	closedir(dir);
	return found;
}

static const char *find_usbfs_path(void) {
	const char *path = "/dev/bus/usb";
	const char *ret = NULL;

	if (check_usb_vfs(path)) {
		ret = path;
	} else {
		path = "/proc/bus/usb";
		if (check_usb_vfs(path))
			ret = path;
	}

	
	if (ret == NULL) {
		struct dirent *entry;
		DIR *dir;

		path = "/dev";
		dir = opendir(path);
		if (dir != NULL) {
			while ((entry = readdir(dir)) != NULL ) {
				if (_is_usbdev_entry(entry, NULL, NULL)) {
					
					ret = path;
					usbdev_names = 1;
					break;
				}
			}
			closedir(dir);
		}
	}

	if (ret != NULL)
		usbi_dbg("found usbfs at %s", ret);

	return ret;
}


static clockid_t find_monotonic_clock(void) {
#ifdef CLOCK_MONOTONIC
	struct timespec ts;
	int r;

	
	r = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (r == 0)
		return CLOCK_MONOTONIC;
	usbi_dbg("monotonic clock doesn't work, errno %d", errno);
#endif

	return CLOCK_REALTIME;
}

static int kernel_version_ge(int major, int minor, int sublevel) {
	struct utsname uts;
	int atoms, kmajor, kminor, ksublevel;

	if (uname(&uts) < 0)
		return -1;
	atoms = sscanf(uts.release, "%d.%d.%d", &kmajor, &kminor, &ksublevel);
	if (UNLIKELY(atoms < 1))
		return -1;

	if (kmajor > major)
		return 1;
	if (kmajor < major)
		return 0;

	
	if (atoms < 2)
		return 0 == minor && 0 == sublevel;
	if (kminor > minor)
		return 1;
	if (kminor < minor)
		return 0;

	
	if (atoms < 3)
		return 0 == sublevel;

	return ksublevel >= sublevel;
}

static int op_init2(struct libusb_context *ctx, const char *usbfs) {	
	struct stat statbuf;
	int r;

	ENTER();
	if (!usbfs || !strlen(usbfs)) {
		usbfs_path = find_usbfs_path();
	} else {
		usbfs_path = usbfs;
	}
	if (UNLIKELY(!usbfs_path)) {
		LOGE("could not find usbfs");
		usbi_err(ctx, "could not find usbfs");
		RETURN(LIBUSB_ERROR_OTHER, int);
	}

	if (monotonic_clkid == -1)
		monotonic_clkid = find_monotonic_clock();

	if (supports_flag_bulk_continuation == -1) {
		
		supports_flag_bulk_continuation = kernel_version_ge(2, 6, 32);
		if (supports_flag_bulk_continuation == -1) {
			LOGE("error checking for bulk continuation support");
			usbi_err(ctx, "error checking for bulk continuation support");
			RETURN(LIBUSB_ERROR_OTHER, int);
		}
	}

	if (supports_flag_bulk_continuation)
		usbi_dbg("bulk continuation flag supported");

	if (-1 == supports_flag_zero_packet) {
		
		supports_flag_zero_packet = kernel_version_ge(2, 6, 31);
		if (-1 == supports_flag_zero_packet) {
			LOGE("error checking for zero length packet support");
			usbi_err(ctx, "error checking for zero length packet support");
			RETURN(LIBUSB_ERROR_OTHER, int);
		}
	}

	if (supports_flag_zero_packet)
		usbi_dbg("zero length packet flag supported");

	if (-1 == sysfs_has_descriptors) {
		
		sysfs_has_descriptors = kernel_version_ge(2, 6, 26);
		if (UNLIKELY(-1 == sysfs_has_descriptors)) {
			LOGE("error checking for sysfs descriptors");
			usbi_err(ctx, "error checking for sysfs descriptors");
			RETURN(LIBUSB_ERROR_OTHER, int);
		}
	}

	if (-1 == sysfs_can_relate_devices) {
		
		sysfs_can_relate_devices = kernel_version_ge(2, 6, 22);
		if (UNLIKELY(-1 == sysfs_can_relate_devices)) {
			LOGE("error checking for sysfs busnum");
			usbi_err(ctx, "error checking for sysfs busnum");
			RETURN(LIBUSB_ERROR_OTHER, int);
		}
	}

	if (sysfs_can_relate_devices || sysfs_has_descriptors) {
		r = stat(SYSFS_DEVICE_PATH, &statbuf);
		if (r != 0 || !S_ISDIR(statbuf.st_mode)) {
			usbi_warn(ctx, "sysfs not mounted");
			sysfs_can_relate_devices = 0;
			sysfs_has_descriptors = 0;
		}
	}

	if (sysfs_can_relate_devices)
		usbi_dbg("sysfs can relate devices");

	if (sysfs_has_descriptors)
		usbi_dbg("sysfs has complete descriptors");

	usbi_mutex_static_lock(&android_hotplug_startstop_lock);
	r = LIBUSB_SUCCESS;
	if (init_count == 0) {
		LOGI("start up hotplug event handler");
		int r = android_start_event_monitor();
		if (r != LIBUSB_SUCCESS) {
			LOGE("warning: error starting hotplug event monitor");
			usbi_err(ctx, "warning: error starting hotplug event monitor");
		}
	}
	if (r == LIBUSB_SUCCESS) {
		LOGI("call android_scan_devices");
		r = android_scan_devices(ctx);
		if (r == LIBUSB_SUCCESS)
			init_count++;
		else if (init_count == 0)
			android_stop_event_monitor();
	} else {
		LOGE("error starting hotplug event monitor");
		usbi_err(ctx, "error starting hotplug event monitor");
	}
	usbi_mutex_static_unlock(&android_hotplug_startstop_lock);

	RETURN(r, int);
}

static int op_init(struct libusb_context *ctx) {
	return op_init2(ctx, NULL);
#if 0
	struct stat statbuf;
	int r;

	usbfs_path = find_usbfs_path();
	if (UNLIKELY(!usbfs_path)) {
		usbi_err(ctx, "could not find usbfs");
		return LIBUSB_ERROR_OTHER;
	}

	if (monotonic_clkid == -1)
		monotonic_clkid = find_monotonic_clock();

	if (supports_flag_bulk_continuation == -1) {
		
		supports_flag_bulk_continuation = kernel_version_ge(2, 6, 32);
		if (supports_flag_bulk_continuation == -1) {
			usbi_err(ctx, "error checking for bulk continuation support");
			return LIBUSB_ERROR_OTHER;
		}
	}

	if (supports_flag_bulk_continuation)
		usbi_dbg("bulk continuation flag supported");

	if (-1 == supports_flag_zero_packet) {
		
		supports_flag_zero_packet = kernel_version_ge(2, 6, 31);
		if (-1 == supports_flag_zero_packet) {
			usbi_err(ctx, "error checking for zero length packet support");
			return LIBUSB_ERROR_OTHER;
		}
	}

	if (supports_flag_zero_packet)
		usbi_dbg("zero length packet flag supported");

	if (-1 == sysfs_has_descriptors) {
		
		sysfs_has_descriptors = kernel_version_ge(2, 6, 26);
		if (UNLIKELY(-1 == sysfs_has_descriptors)) {
			usbi_err(ctx, "error checking for sysfs descriptors");
			return LIBUSB_ERROR_OTHER;
		}
	}

	if (-1 == sysfs_can_relate_devices) {
		
		sysfs_can_relate_devices = kernel_version_ge(2, 6, 22);
		if (UNLIKELY(-1 == sysfs_can_relate_devices)) {
			usbi_err(ctx, "error checking for sysfs busnum");
			return LIBUSB_ERROR_OTHER;
		}
	}

	if (sysfs_can_relate_devices || sysfs_has_descriptors) {
		r = stat(SYSFS_DEVICE_PATH, &statbuf);
		if (r != 0 || !S_ISDIR(statbuf.st_mode)) {
			usbi_warn(ctx, "sysfs not mounted");
			sysfs_can_relate_devices = 0;
			sysfs_has_descriptors = 0;
		}
	}

	if (sysfs_can_relate_devices)
		usbi_dbg("sysfs can relate devices");

	if (sysfs_has_descriptors)
		usbi_dbg("sysfs has complete descriptors");

	usbi_mutex_static_lock(&android_hotplug_startstop_lock);
	r = LIBUSB_SUCCESS;
	if (init_count == 0) {
		LOGI("start up hotplug event handler");
		r = android_start_event_monitor();
	}
	if (r == LIBUSB_SUCCESS) {
		r = android_scan_devices(ctx);
		if (r == LIBUSB_SUCCESS)
			init_count++;
		else if (init_count == 0)
			android_stop_event_monitor();
	} else
		usbi_err(ctx, "error starting hotplug event monitor");
	usbi_mutex_static_unlock(&android_hotplug_startstop_lock);

	return r;
#endif
}


static void op_exit(void) {
	ENTER();

	usbi_mutex_static_lock(&android_hotplug_startstop_lock);
	assert(init_count != 0);
	if (!--init_count) {
		
		(void) android_stop_event_monitor();
	}
	usbi_mutex_static_unlock(&android_hotplug_startstop_lock);

	EXIT();
}

static int android_start_event_monitor(void) {
	ENTER();
#ifdef __ANDROID__
	
	RETURN(LIBUSB_SUCCESS, int);
#else
#if defined(USE_UDEV)
	RETURN(android_udev_start_event_monitor(), int);
#else
	RETURN(android_netlink_start_event_monitor(), int);
#endif
#endif
}

static int android_stop_event_monitor(void) {
	ENTER();
#ifdef __ANDROID__
	RETURN(LIBUSB_SUCCESS, int);
#else
#if defined(USE_UDEV)
	RETURN(android_udev_stop_event_monitor(), int);
#else
	RETURN(android_netlink_stop_event_monitor(), int);
#endif
#endif
}

static int android_scan_devices(struct libusb_context *ctx) {
	ENTER();
	int ret = LIBUSB_SUCCESS;

#ifdef __ANDROID__
	
#else
	usbi_mutex_static_lock(&android_hotplug_lock);

#if defined(USE_UDEV)
	ret = android_udev_scan_devices(ctx);
#else
	ret = android_default_scan_devices(ctx);
#endif

	usbi_mutex_static_unlock(&android_hotplug_lock);
#endif
	RETURN(ret, int);
}

static void op_hotplug_poll(void) {
	ENTER();
#ifdef __ANDROID__
	
#else
#if defined(USE_UDEV)
	android_udev_hotplug_poll();
#else
	android_netlink_hotplug_poll();
#endif
#endif
	EXIT();
}

static int _open_sysfs_attr(struct libusb_device *dev, const char *attr) {
	struct android_device_priv *priv = _device_priv(dev);
	char filename[PATH_MAX];
	int fd;

	snprintf(filename, PATH_MAX, "%s/%s/%s",
		SYSFS_DEVICE_PATH, priv->sysfs_dir, attr);
	fd = open(filename, O_RDONLY);
	if (UNLIKELY(fd < 0)) {
		usbi_err(DEVICE_CTX(dev),
			"open %s failed ret=%d errno=%d", filename, fd, errno);
		return LIBUSB_ERROR_IO;
	}

	return fd;
}


static int __read_sysfs_attr(struct libusb_context *ctx, const char *devname,
		const char *attr) {
	char filename[PATH_MAX];
	FILE *f;
	int r, value;

	snprintf(filename, PATH_MAX, "%s/%s/%s", SYSFS_DEVICE_PATH, devname, attr);
	f = fopen(filename, "r");
	if (UNLIKELY(f == NULL)) {
		if (errno == ENOENT) {
			
			return LIBUSB_ERROR_NO_DEVICE;
		}
		usbi_err(ctx, "open %s failed errno=%d", filename, errno);
		return LIBUSB_ERROR_IO;
	}

	r = fscanf(f, "%d", &value);
	fclose(f);
	if (UNLIKELY(r != 1)) {
		usbi_err(ctx, "fscanf %s returned %d, errno=%d", attr, r, errno);
		return LIBUSB_ERROR_NO_DEVICE; 
	}
	if (UNLIKELY(value < 0)) {
		usbi_err(ctx, "%s contains a negative value", filename);
		return LIBUSB_ERROR_IO;
	}

	return value;
}


static int op_get_raw_descriptor(struct libusb_device *dev,
		unsigned char *buffer, int *descriptors_len, int *host_endian) {
	struct android_device_priv *priv = _device_priv(dev);

	if (!descriptors_len || !host_endian)
		return LIBUSB_ERROR_INVALID_PARAM;
	*host_endian = sysfs_has_descriptors ? 0 : 1;
	if (buffer && (*descriptors_len >= priv->descriptors_len)) {
		memcpy(buffer, priv->descriptors, priv->descriptors_len);
	}
	*descriptors_len = priv->descriptors_len;
	return LIBUSB_SUCCESS;
}

static int op_get_device_descriptor(struct libusb_device *dev,
		unsigned char *buffer, int *host_endian) {
	struct android_device_priv *priv = _device_priv(dev);

	if (!host_endian)
		return LIBUSB_ERROR_INVALID_PARAM;
	*host_endian = sysfs_has_descriptors ? 0 : 1;
	memcpy(buffer, priv->descriptors, DEVICE_DESC_LENGTH);

	return LIBUSB_SUCCESS;
}


static int sysfs_get_active_config(struct libusb_device *dev, int *config) {
	char *endptr;
	char tmp[5] = { 0, 0, 0, 0, 0 };
	long num;
	int fd;
	ssize_t r;

	fd = _open_sysfs_attr(dev, "bConfigurationValue");
	if (UNLIKELY(fd < 0))
		return fd;

	r = read(fd, tmp, sizeof(tmp));
	close(fd);
	if (UNLIKELY(r < 0)) {
		usbi_err(DEVICE_CTX(dev),
			"read bConfigurationValue failed ret=%d errno=%d", r, errno);
		return LIBUSB_ERROR_IO;
	} else if (r == 0) {
		usbi_dbg("device unconfigured");
		*config = -1;
		return LIBUSB_SUCCESS;
	}

	if (tmp[sizeof(tmp) - 1] != 0) {
		usbi_err(DEVICE_CTX(dev), "not null-terminated?");
		return LIBUSB_ERROR_IO;
	} else if (tmp[0] == 0) {
		usbi_err(DEVICE_CTX(dev), "no configuration value?");
		return LIBUSB_ERROR_IO;
	}

	num = strtol(tmp, &endptr, 10);
	if (endptr == tmp) {
		usbi_err(DEVICE_CTX(dev), "error converting '%s' to integer", tmp);
		return LIBUSB_ERROR_IO;
	}

	*config = (int) num;
	return LIBUSB_SUCCESS;
}

int android_get_device_address(struct libusb_context *ctx, int detached,
		uint8_t *busnum, uint8_t *devaddr, const char *dev_node,
		const char *sys_name) {
	int sysfs_attr;

	usbi_dbg("getting address for device: %s detached: %d", sys_name, detached);
	
	if (!sysfs_can_relate_devices || detached || NULL == sys_name) {
		if (NULL == dev_node) {
			return LIBUSB_ERROR_OTHER;
		}

		
		if (!strncmp(dev_node, "/dev/bus/usb", 12)) {
			sscanf(dev_node, "/dev/bus/usb/%hhd/%hhd", busnum, devaddr);
		} else if (!strncmp(dev_node, "/proc/bus/usb", 13)) {
			sscanf(dev_node, "/proc/bus/usb/%hhd/%hhd", busnum, devaddr);
		}

		return LIBUSB_SUCCESS;
	}

	usbi_dbg("scan %s", sys_name);

	sysfs_attr = __read_sysfs_attr(ctx, sys_name, "busnum");
	if (0 > sysfs_attr)
		return sysfs_attr;
	if (sysfs_attr > 255)
		return LIBUSB_ERROR_INVALID_PARAM;
	*busnum = (uint8_t) sysfs_attr;

	sysfs_attr = __read_sysfs_attr(ctx, sys_name, "devnum");
	if (0 > sysfs_attr)
		return sysfs_attr;
	if (sysfs_attr > 255)
		return LIBUSB_ERROR_INVALID_PARAM;

	*devaddr = (uint8_t) sysfs_attr;

	usbi_dbg("bus=%d dev=%d", *busnum, *devaddr);

	return LIBUSB_SUCCESS;
}


static int seek_to_first_descriptor(struct libusb_context *ctx,
		uint8_t descriptor_type, unsigned char *buffer, int size) {
	struct usb_descriptor_header header;
	int i;

	for (i = 0; size >= 0; i += header.bLength, size -= header.bLength) {
		if (size == 0)
			return LIBUSB_ERROR_NOT_FOUND;

		if (size < LIBUSB_DT_HEADER_SIZE) {
			usbi_err(ctx, "short descriptor read %d/2", size);
			return LIBUSB_ERROR_IO;
		}
		usbi_parse_descriptor(buffer + i, "bb", &header, 0);

		if (header.bDescriptorType == descriptor_type)	
			return i;
	}
	usbi_err(ctx, "bLength overflow by %d bytes", -size);
	return LIBUSB_ERROR_IO;
}



static int seek_to_next_descriptor(struct libusb_context *ctx,
		uint8_t descriptor_type, unsigned char *buffer, int size) {
	struct usb_descriptor_header header;
	int i;

	for (i = 0; size >= 0; i += header.bLength, size -= header.bLength) {
		if (size == 0)
			return LIBUSB_ERROR_NOT_FOUND;

		if (size < LIBUSB_DT_HEADER_SIZE) {
			usbi_err(ctx, "short descriptor read %d/2", size);
			return LIBUSB_ERROR_IO;
		}
		usbi_parse_descriptor(buffer + i, "bb", &header, 0);

		if (i && header.bDescriptorType == descriptor_type)
			return i;
	}
	usbi_err(ctx, "bLength overflow by %d bytes", -size);
	return LIBUSB_ERROR_IO;
}


static int seek_to_next_config(struct libusb_context *ctx,
		unsigned char *buffer, int size) {
	struct libusb_config_descriptor config;
	struct usb_descriptor_header header;

	if (size == 0)
		return LIBUSB_ERROR_NOT_FOUND;

	if (size < LIBUSB_DT_HEADER_SIZE) {
		usbi_err(ctx, "short descriptor read %d/%d",
			size, LIBUSB_DT_CONFIG_SIZE);
		return LIBUSB_ERROR_IO;
	}
	if (size < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(ctx, "short descriptor read %d/%d",
			size, LIBUSB_DT_CONFIG_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(buffer, "bbwbbbbb", &config, 0);
	if (config.bDescriptorType != LIBUSB_DT_CONFIG) {
		usbi_err(ctx, "descriptor is not a config desc (type 0x%02x)",
			config.bDescriptorType);
		return LIBUSB_ERROR_IO;
	}

	
	if (sysfs_has_descriptors) {
		int next = seek_to_next_descriptor(ctx, LIBUSB_DT_CONFIG, buffer, size);
		if (next == LIBUSB_ERROR_NOT_FOUND)
			next = size;
		if (next < 0)
			return next;

		if (next != config.wTotalLength)
			usbi_warn(ctx, "config length mismatch wTotalLength "
				"%d real %d", config.wTotalLength, next);
		return next;
	} else {
		if (config.wTotalLength < LIBUSB_DT_CONFIG_SIZE) {
			usbi_err(ctx, "invalid wTotalLength %d",
				config.wTotalLength);
			return LIBUSB_ERROR_IO;
		} else if (config.wTotalLength > size) {
			usbi_warn(ctx, "short descriptor read %d/%d",
				size, config.wTotalLength);
			return size;
		} else
			return config.wTotalLength;
	}
}

static int op_get_config_descriptor_by_value(struct libusb_device *dev,
		uint8_t value, unsigned char **buffer, int *host_endian) {
	struct libusb_context *ctx = DEVICE_CTX(dev);
	struct android_device_priv *priv = _device_priv(dev);
	unsigned char *descriptors = priv->descriptors;
	int size = priv->descriptors_len, r;
	struct libusb_config_descriptor *config;

	*buffer = NULL;
	
	*host_endian = 0;

	
	descriptors += DEVICE_DESC_LENGTH;
	size -= DEVICE_DESC_LENGTH;
	
	
	
	
	
	
	r = seek_to_first_descriptor(ctx, LIBUSB_DT_CONFIG, descriptors, size);
	if UNLIKELY(r < 0) {
		LOGE("could not find config descriptor:r=%d", r);
		return r;
	}
	descriptors += r;
	size -= r;
	
	for (; ;) {
		register int next = seek_to_next_config(ctx, descriptors, size);
		if UNLIKELY(next < 0)
			return next;
		config = (struct libusb_config_descriptor *) descriptors;
		if (config->bConfigurationValue == value) {
			*buffer = descriptors;
			return next;
		}
		size -= next;
		descriptors += next;
	}
}

static int op_get_active_config_descriptor(struct libusb_device *dev,
		unsigned char *buffer, size_t len, int *host_endian) {
	int r, config;
	unsigned char *config_desc;

	if (sysfs_can_relate_devices) {
		r = sysfs_get_active_config(dev, &config);
		if (UNLIKELY(r < 0))
			return r;
	} else {
		
		struct android_device_priv *priv = _device_priv(dev);
		config = priv->active_config;
	}
	if (config == -1)
		return LIBUSB_ERROR_NOT_FOUND;

	r = op_get_config_descriptor_by_value(dev, config, &config_desc,
			host_endian);
	if (UNLIKELY(r < 0))
		return r;

	len = MIN(len, r);
	memcpy(buffer, config_desc, len);
	return len;
}

static int op_get_config_descriptor(struct libusb_device *dev,
		uint8_t config_index, unsigned char *buffer, size_t len,
		int *host_endian) {
	struct libusb_context *ctx = DEVICE_CTX(dev);
	struct android_device_priv *priv = _device_priv(dev);
	unsigned char *descriptors = priv->descriptors;
	int i, r, size = priv->descriptors_len;

	
	*host_endian = 0;

	
	descriptors += DEVICE_DESC_LENGTH;
	size -= DEVICE_DESC_LENGTH;
	
	
	
	
	
	
	r = seek_to_first_descriptor(ctx, LIBUSB_DT_CONFIG, descriptors, size);
	if UNLIKELY(r < 0) {
		LOGE("could not find config descriptor:r=%d", r);
		return r;
	}
	descriptors += r;
	size -= r;
	
	for (i = 0; ; i++) {
		r = seek_to_next_config(ctx, descriptors, size);
		if (UNLIKELY(r < 0))	
			return r;
		if (i == config_index)
			break;
		size -= r;
		descriptors += r;
	}

	len = MIN(len, r);
	memcpy(buffer, descriptors, len);
	return len;
}


static int usbfs_get_active_config(struct libusb_device *dev, int fd) {
	unsigned char active_config = 0;
	int r;

	struct usbfs_ctrltransfer ctrl = {
		.bmRequestType = LIBUSB_ENDPOINT_IN,
		.bRequest = LIBUSB_REQUEST_GET_CONFIGURATION,
		.wValue = 0,
		.wIndex = 0,
		.wLength = 1,
		.timeout = 1000,
		.data = &active_config
	};

	r = ioctl(fd, IOCTL_USBFS_CONTROL, &ctrl);
	if (UNLIKELY(r < 0)) {
		if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		
		usbi_warn(DEVICE_CTX(dev),
			"get_configuration failed ret=%d errno=%d", r, errno);
		return LIBUSB_ERROR_IO;
	}

	return active_config;
}

static int initialize_device(struct libusb_device *dev, uint8_t busnum,
		uint8_t devaddr, const char *sysfs_dir) {

	struct android_device_priv *priv = _device_priv(dev);
	struct libusb_context *ctx = DEVICE_CTX(dev);
	int descriptors_size = 512; 
	int fd, speed;
	ssize_t r;

	dev->bus_number = busnum;
	dev->device_address = devaddr;

	if (sysfs_dir) {
		priv->sysfs_dir = malloc(strlen(sysfs_dir) + 1);
		if (!priv->sysfs_dir)
			return LIBUSB_ERROR_NO_MEM;
		strcpy(priv->sysfs_dir, sysfs_dir);

		
		speed = __read_sysfs_attr(DEVICE_CTX(dev), sysfs_dir, "speed");
		if (speed >= 0) {
			switch (speed) {
			case    1: dev->speed = LIBUSB_SPEED_LOW;	break;
			case   12: dev->speed = LIBUSB_SPEED_FULL;	break;
			case  480: dev->speed = LIBUSB_SPEED_HIGH;	break;
			case 5000: dev->speed = LIBUSB_SPEED_SUPER;	break;
			default:
				usbi_warn(DEVICE_CTX(dev), "Unknown device speed: %d Mbps", speed);
			}
		}
	}

	
	if (sysfs_has_descriptors) {
		fd = _open_sysfs_attr(dev, "descriptors");
	} else {
		fd = _get_usbfs_fd(dev, O_RDONLY, 0);
	}
	if (fd < 0)
		return fd;

	do {
		descriptors_size *= 2;
		priv->descriptors = usbi_reallocf(priv->descriptors, descriptors_size);
		if (UNLIKELY(!priv->descriptors)) {
			close(fd);
			return LIBUSB_ERROR_NO_MEM;
		}
		
		if (!sysfs_has_descriptors) {
			memset(priv->descriptors + priv->descriptors_len, 0,
					descriptors_size - priv->descriptors_len);
		}
		r = read(fd, priv->descriptors + priv->descriptors_len,
				descriptors_size - priv->descriptors_len);
		if (UNLIKELY(r < 0)) {
			usbi_err(ctx, "read descriptor failed ret=%d errno=%d", fd, errno);
			close(fd);
			return LIBUSB_ERROR_IO;
		}
		priv->descriptors_len += r;
	} while (priv->descriptors_len == descriptors_size);

	close(fd);

	if (UNLIKELY(priv->descriptors_len < DEVICE_DESC_LENGTH)) {
		usbi_err(ctx, "short descriptor read (%d)", priv->descriptors_len);
		return LIBUSB_ERROR_IO;
	}

	if (sysfs_can_relate_devices)
		return LIBUSB_SUCCESS;

	
	fd = _get_usbfs_fd(dev, O_RDWR, 1);
	if (fd < 0) {	
		
		usbi_warn(ctx, "Missing rw usbfs access; cannot determine "
				"active configuration descriptor");
		if (priv->descriptors_len
				>= (DEVICE_DESC_LENGTH + LIBUSB_DT_CONFIG_SIZE)) {
			struct libusb_config_descriptor config;
			usbi_parse_descriptor(priv->descriptors + DEVICE_DESC_LENGTH,
				"bbwbbbbb", &config, 0);
			priv->active_config = config.bConfigurationValue;
		} else
			priv->active_config = -1; 

		return LIBUSB_SUCCESS;
	}
	
	r = usbfs_get_active_config(dev, fd);
	if (r > 0) {
		priv->active_config = r;
		r = LIBUSB_SUCCESS;
	} else if (r == 0) {
		
		usbi_dbg("active cfg 0? assuming unconfigured device");
		priv->active_config = -1;
		r = LIBUSB_SUCCESS;
	} else if (r == LIBUSB_ERROR_IO) {
		
		usbi_warn(ctx, "couldn't query active configuration, assuming"
					" unconfigured");
		priv->active_config = -1;
		r = LIBUSB_SUCCESS;
	} 

	close(fd);
	return r;
}

static int android_get_parent_info(struct libusb_device *dev,
		const char *sysfs_dir) {
	struct libusb_context *ctx = DEVICE_CTX(dev);
	struct libusb_device *it;
	char *parent_sysfs_dir, *tmp;
	int ret, add_parent = 1;

	
	if (NULL == sysfs_dir || 0 == strncmp(sysfs_dir, "usb", 3)) {
		
		return LIBUSB_SUCCESS;
	}

	parent_sysfs_dir = strdup(sysfs_dir);
	if (NULL != (tmp = strrchr(parent_sysfs_dir, '.')) ||
	NULL != (tmp = strrchr(parent_sysfs_dir, '-'))) {
		dev->port_number = atoi(tmp + 1);
		*tmp = '\0';
	} else {
		usbi_warn(ctx, "Can not parse sysfs_dir: %s, no parent info",
			parent_sysfs_dir);
		free(parent_sysfs_dir);
		return LIBUSB_SUCCESS;
	}

	
	if (NULL == strchr(parent_sysfs_dir, '-')) {
		tmp = parent_sysfs_dir;
		ret = asprintf(&parent_sysfs_dir, "usb%s", tmp);
		free(tmp);
		if (0 > ret) {
			return LIBUSB_ERROR_NO_MEM;
		}
	}

retry:
	
	usbi_mutex_lock(&ctx->usb_devs_lock);
	list_for_each_entry(it, &ctx->usb_devs, list, struct libusb_device)
	{
		struct android_device_priv *priv = _device_priv(it);
		if (0 == strcmp(priv->sysfs_dir, parent_sysfs_dir)) {
			dev->parent_dev = libusb_ref_device(it);
			break;
		}
	}
	usbi_mutex_unlock(&ctx->usb_devs_lock);

	if (!dev->parent_dev && add_parent) {
		usbi_dbg("parent_dev %s not enumerated yet, enumerating now",
			parent_sysfs_dir);
		sysfs_scan_device(ctx, parent_sysfs_dir);
		add_parent = 0;
		goto retry;
	}

	usbi_dbg("Dev %p (%s) has parent %p (%s) port %d", dev, sysfs_dir,
		dev->parent_dev, parent_sysfs_dir, dev->port_number);

	free(parent_sysfs_dir);

	return LIBUSB_SUCCESS;
}

static int android_initialize_device(struct libusb_device *dev,
	uint8_t busnum, uint8_t devaddr, int fd) {

	ENTER();

	struct android_device_priv *priv = _device_priv(dev);
	struct libusb_context *ctx = DEVICE_CTX(dev);
	uint8_t desc[4096]; 
	int speed;
	ssize_t r;

	dev->bus_number = busnum;
	dev->device_address = devaddr;

	LOGD("cache descriptors in memory");

	priv->descriptors_len = 0;
	priv->fd = 0;
	memset(desc, 0, sizeof(desc));
    if (!lseek(fd, 0, SEEK_SET)) {
        
        int length = read(fd, desc, sizeof(desc));
        LOGD("Device::init read returned %d errno %d\n", length, errno);
		if (length > 0) {
			priv->fd = fd;
			priv->descriptors = usbi_reallocf(priv->descriptors, length);
			if (UNLIKELY(!priv->descriptors)) {
				RETURN(LIBUSB_ERROR_NO_MEM, int);
			}
			priv->descriptors_len = length;
			memcpy(priv->descriptors, desc, length);
		}
	}

	if (UNLIKELY(priv->descriptors_len < DEVICE_DESC_LENGTH)) {
		usbi_err(ctx, "short descriptor read (%d)", priv->descriptors_len);
		LOGE("short descriptor read (%d)", priv->descriptors_len);
		RETURN(LIBUSB_ERROR_IO, int);
	}

	if (fd < 0) {	
		
		usbi_warn(ctx, "Missing rw usbfs access; cannot determine "
				"active configuration descriptor");
		if (priv->descriptors_len
				>= (DEVICE_DESC_LENGTH + LIBUSB_DT_CONFIG_SIZE)) {
			struct libusb_config_descriptor config;
			usbi_parse_descriptor(priv->descriptors + DEVICE_DESC_LENGTH,
				"bbwbbbbb", &config, 0);
			priv->active_config = config.bConfigurationValue;
		} else
			priv->active_config = -1; 

		RETURN(LIBUSB_SUCCESS, int);
	}
	
	r = usbfs_get_active_config(dev, fd);
	if (r > 0) {
		priv->active_config = r;
		r = LIBUSB_SUCCESS;
	} else if (r == 0) {
		
		usbi_dbg("active cfg 0? assuming unconfigured device");
		priv->active_config = -1;
		r = LIBUSB_SUCCESS;
	} else if (r == LIBUSB_ERROR_IO) {
		
		usbi_warn(ctx, "couldn't query active configuration, assuming"
					" unconfigured");
		priv->active_config = -1;
		r = LIBUSB_SUCCESS;
	} 

	RETURN(r, int);
}

int android_generate_device(struct libusb_context *ctx, struct libusb_device **dev,
	int vid, int pid, const char *serial, int fd, int busnum, int devaddr) {

	ENTER();

	unsigned long session_id;
	int r = 0;

	*dev = NULL;
 	
	session_id = busnum << 8 | devaddr;
 	LOGD("allocating new device for %d/%d (session %ld)", busnum, devaddr, session_id);
 	*dev = usbi_alloc_device(ctx, session_id);	
 	if (UNLIKELY(!dev)) {
 		RETURN(LIBUSB_ERROR_NO_MEM, int);
 	}

 	r = android_initialize_device(*dev, busnum, devaddr, fd);
 	if (UNLIKELY(r < 0)) {
 		LOGE("initialize_device failed: ret=%d", r);
 		goto out;
 	}
 	r = usbi_sanitize_device(*dev);
 	if (UNLIKELY(r < 0)) {
 		LOGE("usbi_sanitize_device failed: ret=%d", r);
 		goto out;
 	}

out:
 	if (UNLIKELY(r < 0)) {
 		libusb_unref_device(*dev);	
 		*dev = NULL;
 	} else {
 		usbi_connect_device(*dev);
 	}

 	RETURN(r, int);
}


int android_enumerate_device(struct libusb_context *ctx, uint8_t busnum,
		uint8_t devaddr, const char *sysfs_dir) {

	unsigned long session_id;
	struct libusb_device *dev;
	int r = 0;

	
	session_id = busnum << 8 | devaddr;
	usbi_dbg("busnum %d devaddr %d session_id %ld",
		busnum, devaddr, session_id);

	dev = usbi_get_device_by_session_id(ctx, session_id);
	if (dev) {
		
		usbi_dbg("session_id %ld already exists", session_id);
		libusb_unref_device(dev);
		return LIBUSB_SUCCESS;
	}

	usbi_dbg("allocating new device for %d/%d (session %ld)",
		busnum, devaddr, session_id);
	dev = usbi_alloc_device(ctx, session_id);
	if (UNLIKELY(!dev))
		return LIBUSB_ERROR_NO_MEM;

	r = initialize_device(dev, busnum, devaddr, sysfs_dir);
	if (UNLIKELY(r < 0))
		goto out;
	r = usbi_sanitize_device(dev);
	if (UNLIKELY(r < 0))
		goto out;

	r = android_get_parent_info(dev, sysfs_dir);
	if (UNLIKELY(r < 0))
		goto out;
out:
	if (UNLIKELY(r < 0))
		libusb_unref_device(dev);
	else
		usbi_connect_device(dev);

	return r;
}

void android_hotplug_enumerate(uint8_t busnum, uint8_t devaddr,
		const char *sys_name) {
	struct libusb_context *ctx;

	usbi_mutex_static_lock(&active_contexts_lock);
	list_for_each_entry(ctx, &active_contexts_list, list, struct libusb_context)
	{
		android_enumerate_device(ctx, busnum, devaddr, sys_name);
	}
	usbi_mutex_static_unlock(&active_contexts_lock);
}

void android_device_disconnected(uint8_t busnum, uint8_t devaddr,
		const char *sys_name) {
	struct libusb_context *ctx;
	struct libusb_device *dev;
	unsigned long session_id = busnum << 8 | devaddr;

	usbi_mutex_static_lock(&active_contexts_lock);
	list_for_each_entry(ctx, &active_contexts_list, list, struct libusb_context)
	{
		dev = usbi_get_device_by_session_id(ctx, session_id);
		if (NULL != dev) {
			usbi_disconnect_device(dev);
			libusb_unref_device(dev);
		} else {
			usbi_dbg("device not found for session %x", session_id);
		}
	}
	usbi_mutex_static_unlock(&active_contexts_lock);
}

#if !defined(USE_UDEV)

static int usbfs_scan_busdir(struct libusb_context *ctx, uint8_t busnum) {
	DIR *dir;
	char dirpath[PATH_MAX];
	struct dirent *entry;
	int r = LIBUSB_ERROR_IO;

	snprintf(dirpath, PATH_MAX, "%s/%03d", usbfs_path, busnum);
	usbi_dbg("%s", dirpath);
	dir = opendir(dirpath);
	if (UNLIKELY(!dir)) {
		usbi_err(ctx, "opendir '%s' failed, errno=%d", dirpath, errno);
		
		return r;
	}

	while ((entry = readdir(dir))) {
		int devaddr;

		if (entry->d_name[0] == '.')
			continue;

		devaddr = atoi(entry->d_name);
		if (devaddr == 0) {
			usbi_dbg("unknown dir entry %s", entry->d_name);
			continue;
		}

		if (android_enumerate_device(ctx, busnum, (uint8_t) devaddr, NULL)) {
			usbi_dbg("failed to enumerate dir entry %s", entry->d_name);
			continue;
		}

		r = 0;
	}

	closedir(dir);
	return r;
}

static int usbfs_get_device_list(struct libusb_context *ctx) {
	struct dirent *entry;
	DIR *buses = opendir(usbfs_path);
	int r = 0;

	if (!buses) {
		usbi_err(ctx, "opendir buses failed errno=%d", errno);
		return LIBUSB_ERROR_IO;
	}

	while ((entry = readdir(buses))) {
		int busnum;

		if (entry->d_name[0] == '.')
			continue;

		if (usbdev_names) {
			int devaddr;
			if (!_is_usbdev_entry(entry, &busnum, &devaddr))
				continue;

			r = android_enumerate_device(ctx, busnum, (uint8_t) devaddr, NULL);
			if (UNLIKELY(r < 0)) {
				usbi_dbg("failed to enumerate dir entry %s", entry->d_name);
				continue;
			}
		} else {
			busnum = atoi(entry->d_name);
			if (UNLIKELY(busnum == 0)) {
				usbi_dbg("unknown dir entry %s", entry->d_name);
				continue;
			}

			r = usbfs_scan_busdir(ctx, busnum);
			if (UNLIKELY(r < 0))
				break;
		}
	}

	closedir(buses);
	return r;

}
#endif

static int sysfs_scan_device(struct libusb_context *ctx, const char *devname) {
	uint8_t busnum, devaddr;
	int ret;

	ret = android_get_device_address(ctx, 0, &busnum, &devaddr, NULL, devname);
	if (UNLIKELY(LIBUSB_SUCCESS != ret)) {
		return ret;
	}

	return android_enumerate_device(ctx, busnum & 0xff, devaddr & 0xff, devname);
}

#if !defined(USE_UDEV)
static int sysfs_get_device_list(struct libusb_context *ctx) {
	DIR *devices = opendir(SYSFS_DEVICE_PATH);
	struct dirent *entry;
	int r = LIBUSB_ERROR_IO;

	if (UNLIKELY(!devices)) {
		usbi_err(ctx, "opendir devices failed errno=%d", errno);
		return r;
	}

	while ((entry = readdir(devices))) {
		if ((!isdigit(entry->d_name[0]) && strncmp(entry->d_name, "usb", 3))
				|| strchr(entry->d_name, ':'))
			continue;

		if (sysfs_scan_device(ctx, entry->d_name)) {
			usbi_dbg("failed to enumerate dir entry %s", entry->d_name);
			continue;
		}

		r = 0;
	}

	closedir(devices);
	return r;
}

static int android_default_scan_devices(struct libusb_context *ctx) {
	
	if (sysfs_can_relate_devices != 0)
		return sysfs_get_device_list(ctx);
	else
		return usbfs_get_device_list(ctx);
}
#endif




static int op_set_device_fd(struct libusb_device *device, int fd) {
	struct android_device_priv *dpriv = _device_priv(device);
	dpriv->fd = fd;
	return LIBUSB_SUCCESS;
}

static int op_open(struct libusb_device_handle *handle) {
	struct android_device_handle_priv *hpriv = _device_handle_priv(handle);
	int r;

	hpriv->fd = _get_usbfs_fd(handle->dev, O_RDWR, 0);
	if (hpriv->fd < 0) {
		if (hpriv->fd == LIBUSB_ERROR_NO_DEVICE) {
			
			usbi_mutex_static_lock(&android_hotplug_lock);
			if (handle->dev->attached) {
				usbi_dbg("open failed with no device, but device still attached");
				android_device_disconnected(handle->dev->bus_number,
						handle->dev->device_address, NULL);
			}
			usbi_mutex_static_unlock(&android_hotplug_lock);
		}
		return hpriv->fd;
	}

	r = ioctl(hpriv->fd, IOCTL_USBFS_GET_CAPABILITIES, &hpriv->caps);
	if (UNLIKELY(r < 0)) {
		if (errno == ENOTTY)
			usbi_dbg("getcap not available");
		else
			usbi_err(HANDLE_CTX(handle), "getcap failed (%d)", errno);
		hpriv->caps = 0;
		if (supports_flag_zero_packet)
			hpriv->caps |= USBFS_CAP_ZERO_PACKET;
		if (supports_flag_bulk_continuation)
			hpriv->caps |= USBFS_CAP_BULK_CONTINUATION;
	}

	return usbi_add_pollfd(HANDLE_CTX(handle), hpriv->fd, POLLOUT);
}

static void op_close(struct libusb_device_handle *dev_handle) {
	int fd = _device_handle_priv(dev_handle)->fd;
	usbi_remove_pollfd(HANDLE_CTX(dev_handle), fd);
#ifndef __ANDROID__
	
	
	close(fd);
#endif
}

static int op_get_configuration(struct libusb_device_handle *handle,
		int *config) {
	int r;

	if (sysfs_can_relate_devices) {
		r = sysfs_get_active_config(handle->dev, config);
	} else {
		r = usbfs_get_active_config(handle->dev,
				_device_handle_priv(handle)->fd);
	}
	if (UNLIKELY(r < 0))
		return r;

	if (*config == -1) {
		usbi_err(HANDLE_CTX(handle), "device unconfigured");
		*config = 0;
	}

	return LIBUSB_SUCCESS;
}

static int op_set_configuration(struct libusb_device_handle *handle, int config) {
	struct android_device_priv *priv = _device_priv(handle->dev);
	const int fd = _device_handle_priv(handle)->fd;
	int r = ioctl(fd, IOCTL_USBFS_SETCONFIG, &config);
	if (UNLIKELY(r)) {
		if (errno == EINVAL) {
			return LIBUSB_ERROR_NOT_FOUND;
		} else if (errno == EBUSY) {
			return LIBUSB_ERROR_BUSY;
		} else if (errno == ENODEV) {
			return LIBUSB_ERROR_NO_DEVICE;
		}
		usbi_err(HANDLE_CTX(handle), "failed, error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}

	
	priv->active_config = config;

	return LIBUSB_SUCCESS;
}

static int claim_interface(struct libusb_device_handle *handle, int iface) {

	ENTER();

	const int fd = _device_handle_priv(handle)->fd;
	LOGD("interface=%d, fd=%d", iface, fd);

	int r = ioctl(fd, IOCTL_USBFS_CLAIMINTF, &iface);
	if (UNLIKELY(r)) {
		if (errno == ENOENT) {
			RETURN(LIBUSB_ERROR_NOT_FOUND, int);
		} else if (errno == EBUSY) {
			RETURN(LIBUSB_ERROR_BUSY, int);
		} else if (errno == ENODEV) {
			RETURN(LIBUSB_ERROR_NO_DEVICE, int);
	}
		LOGE("claim interface failed, error %d errno %d", r, errno);
		RETURN(LIBUSB_ERROR_OTHER, int);
	}

	RETURN(LIBUSB_SUCCESS, int);
}

static int release_interface(struct libusb_device_handle *handle, int iface) {

	ENTER();

	const int fd = _device_handle_priv(handle)->fd;
	LOGD("interface=%d, fd=%d", iface, fd);

	int r = ioctl(fd, IOCTL_USBFS_RELEASEINTF, &iface);
	if (UNLIKELY(r)) {
		if (errno == ENODEV) {
			RETURN(LIBUSB_ERROR_NO_DEVICE, int);
	}
		LOGE("release interface failed, error %d errno %d", r, errno);
		RETURN(LIBUSB_ERROR_OTHER, int);
	}

	RETURN(LIBUSB_SUCCESS, int);
}

static int op_set_interface(struct libusb_device_handle *handle, int iface, int altsetting) {

	ENTER();

	const int fd = _device_handle_priv(handle)->fd;
	struct usbfs_setinterface setintf;
	int r;

	setintf.interface = iface;
	setintf.altsetting = altsetting;
	r = ioctl(fd, IOCTL_USBFS_SETINTF, &setintf);
	if (UNLIKELY(r)) {
		if (errno == EINVAL) {
			RETURN(LIBUSB_ERROR_NOT_FOUND, int);
		} else if (errno == ENODEV) {
			RETURN(LIBUSB_ERROR_NO_DEVICE, int);
		}
		usbi_err(HANDLE_CTX(handle),
			"setintf failed error %d errno %d", r, errno);
		RETURN(LIBUSB_ERROR_OTHER, int);
	}

	RETURN(LIBUSB_SUCCESS, int);
}

static int op_clear_halt(struct libusb_device_handle *handle,
		unsigned char endpoint) {
	const int fd = _device_handle_priv(handle)->fd;
	unsigned int _endpoint = endpoint;
	int r = ioctl(fd, IOCTL_USBFS_CLEAR_HALT, &_endpoint);
	if (UNLIKELY(r)) {
		if (errno == ENOENT)
			return LIBUSB_ERROR_NOT_FOUND;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle),
			"clear_halt failed error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}

	return LIBUSB_SUCCESS;
}

static int op_reset_device(struct libusb_device_handle *handle) {
	const int fd = _device_handle_priv(handle)->fd;
	int i, r, ret = 0;

	
	for (i = 0; i < USB_MAXINTERFACES; i++) {
		if (handle->claimed_interfaces & (1L << i)) {
			release_interface(handle, i);
		}
	}

	usbi_mutex_lock(&handle->lock);
	r = ioctl(fd, IOCTL_USBFS_RESET, NULL);
	if (UNLIKELY(r)) {
		if (errno == ENODEV) {
			ret = LIBUSB_ERROR_NOT_FOUND;
			goto out;
		}

		usbi_err(HANDLE_CTX(handle),
			"reset failed error %d errno %d", r, errno);
		ret = LIBUSB_ERROR_OTHER;
		goto out;
	}

	
	for (i = 0; i < USB_MAXINTERFACES; i++) {
		if (handle->claimed_interfaces & (1L << i)) {
			
			r = detach_kernel_driver_and_claim(handle, i);
			if (UNLIKELY(r)) {
				usbi_warn(HANDLE_CTX(handle),
					"failed to re-claim interface %d after reset: %s",
					i, libusb_error_name(r));
				handle->claimed_interfaces &= ~(1L << i);
				ret = LIBUSB_ERROR_NOT_FOUND;
			}
		}
	}
out:
	usbi_mutex_unlock(&handle->lock);
	return ret;
}

static int do_streams_ioctl(struct libusb_device_handle *handle, long req,
	uint32_t num_streams, unsigned char *endpoints, int num_endpoints) {
	const int fd = _device_handle_priv(handle)->fd;
	int r;
	struct usbfs_streams *streams;

	if (num_endpoints > 30) 
		return LIBUSB_ERROR_INVALID_PARAM;

	streams = malloc(sizeof(struct usbfs_streams) + num_endpoints);
	if (!streams)
		return LIBUSB_ERROR_NO_MEM;

	streams->num_streams = num_streams;
	streams->num_eps = num_endpoints;
	memcpy(streams->eps, endpoints, num_endpoints);

	r = ioctl(fd, req, streams);

	free(streams);

	if (r < 0) {
		if (errno == ENOTTY)
			return LIBUSB_ERROR_NOT_SUPPORTED;
		else if (errno == EINVAL)
			return LIBUSB_ERROR_INVALID_PARAM;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle),
			"streams-ioctl failed error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}
	return r;
}

static int op_alloc_streams(struct libusb_device_handle *handle,
	uint32_t num_streams, unsigned char *endpoints, int num_endpoints)
{
	return do_streams_ioctl(handle, IOCTL_USBFS_ALLOC_STREAMS,
				num_streams, endpoints, num_endpoints);
}

static int op_free_streams(struct libusb_device_handle *handle,
		unsigned char *endpoints, int num_endpoints)
{
	return do_streams_ioctl(handle, IOCTL_USBFS_FREE_STREAMS, 0,
				endpoints, num_endpoints);
}

static int op_kernel_driver_active(struct libusb_device_handle *handle, int interface) {
	const int fd = _device_handle_priv(handle)->fd;
	struct usbfs_getdriver getdrv;
	int r;

	getdrv.interface = interface;
	r = ioctl(fd, IOCTL_USBFS_GETDRIVER, &getdrv);
	if (UNLIKELY(r)) {
		if (errno == ENODATA)
			return LIBUSB_SUCCESS;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle),
			"get driver failed error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}

	return (strcmp(getdrv.driver, "usbfs") == 0) ? 0 : 1;
}

static int op_detach_kernel_driver(struct libusb_device_handle *handle, int interface) {
	const int fd = _device_handle_priv(handle)->fd;
	struct usbfs_ioctl command;
	struct usbfs_getdriver getdrv;
	int r;

	command.ifno = interface;
	command.ioctl_code = IOCTL_USBFS_DISCONNECT;
	command.data = NULL;

	getdrv.interface = interface;
	r = ioctl(fd, IOCTL_USBFS_GETDRIVER, &getdrv);
	if (r == 0 && strcmp(getdrv.driver, "usbfs") == 0)
		return LIBUSB_ERROR_NOT_FOUND;

	r = ioctl(fd, IOCTL_USBFS_IOCTL, &command);
	if (UNLIKELY(r)) {
		if (errno == ENODATA)
			return LIBUSB_ERROR_NOT_FOUND;
		else if (errno == EINVAL)
			return LIBUSB_ERROR_INVALID_PARAM;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle),
			"detach failed error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	}

	return LIBUSB_SUCCESS;
}

static int op_attach_kernel_driver(struct libusb_device_handle *handle, int interface) {
	const int fd = _device_handle_priv(handle)->fd;
	struct usbfs_ioctl command;
	int r;

	command.ifno = interface;
	command.ioctl_code = IOCTL_USBFS_CONNECT;
	command.data = NULL;

	r = ioctl(fd, IOCTL_USBFS_IOCTL, &command);
	if (UNLIKELY(r < 0)) {
		if (errno == ENODATA)
			return LIBUSB_ERROR_NOT_FOUND;
		else if (errno == EINVAL)
			return LIBUSB_ERROR_INVALID_PARAM;
		else if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;
		else if (errno == EBUSY)
			return LIBUSB_ERROR_BUSY;

		usbi_err(HANDLE_CTX(handle),
			"attach failed error %d errno %d", r, errno);
		return LIBUSB_ERROR_OTHER;
	} else if (UNLIKELY(r == 0)) {
		return LIBUSB_ERROR_NOT_FOUND;
	}

	return LIBUSB_SUCCESS;
}

static int detach_kernel_driver_and_claim(struct libusb_device_handle *handle, int interface) {

	ENTER();

	const int fd = _device_handle_priv(handle)->fd;
	struct usbfs_disconnect_claim dc;
	int r;

	dc.interface = interface;
	strcpy(dc.driver, "usbfs");
	dc.flags = USBFS_DISCONNECT_CLAIM_EXCEPT_DRIVER;
	r = ioctl(fd, IOCTL_USBFS_DISCONNECT_CLAIM, &dc);
	if (r == 0 || (r != 0 && errno != ENOTTY)) {
		if (r == 0) {
			RETURN(LIBUSB_SUCCESS, int);
		}

		switch (errno) {
		case EBUSY:
			RETURN(LIBUSB_ERROR_BUSY, int);
		case EINVAL:
			RETURN(LIBUSB_ERROR_INVALID_PARAM, int);
		case ENODEV:
			RETURN(LIBUSB_ERROR_NO_DEVICE, int);
		}
		usbi_err(HANDLE_CTX(handle),
			"disconnect-and-claim failed errno %d", errno);
		RETURN(LIBUSB_ERROR_OTHER, int);
	}

	
	r = op_detach_kernel_driver(handle, interface);
	if (r != 0 && r != LIBUSB_ERROR_NOT_FOUND) {
		RETURN(r, int);
	}

	r = claim_interface(handle, interface);
	RETURN(r, int);
}

static int op_claim_interface(struct libusb_device_handle *handle, int iface) {
	if (handle->auto_detach_kernel_driver)
		return detach_kernel_driver_and_claim(handle, iface);
	else
		return claim_interface(handle, iface);
}

static int op_release_interface(struct libusb_device_handle *handle, int iface) {
	int r;

	r = release_interface(handle, iface);
	if (UNLIKELY(r))
		return r;

	if (handle->auto_detach_kernel_driver)
		op_attach_kernel_driver(handle, iface);

	return LIBUSB_SUCCESS;
}

static void op_destroy_device(struct libusb_device *dev) {
	struct android_device_priv *priv = _device_priv(dev);
	if (priv->descriptors)
		free(priv->descriptors);
	if (priv->sysfs_dir)
		free(priv->sysfs_dir);
}


static int discard_urbs(struct usbi_transfer *itransfer, int first, int last_plus_one) {
	ENTER();

	struct libusb_transfer *transfer = USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);
	struct android_device_handle_priv *dpriv = _device_handle_priv(transfer->dev_handle);
	int i, ret = 0;
	struct usbfs_urb *urb;

	for (i = last_plus_one - 1; i >= first; i--) {
		if (LIBUSB_TRANSFER_TYPE_ISOCHRONOUS == transfer->type)
			urb = tpriv->iso_urbs[i];
		else
			urb = &tpriv->urbs[i];

		
		if (0 == ioctl(dpriv->fd, IOCTL_USBFS_DISCARDURB, urb))
			continue;

		if (EINVAL == errno) {
			usbi_dbg("URB not found --> assuming ready to be reaped");
			if (i == (last_plus_one - 1))
				ret = LIBUSB_ERROR_NOT_FOUND;
		} else if (ENODEV == errno) {
			usbi_dbg("Device not found for URB --> assuming ready to be reaped");
			ret = LIBUSB_ERROR_NO_DEVICE;
		} else {
			usbi_warn(TRANSFER_CTX(transfer),
				"unrecognised discard errno %d", errno);
			ret = LIBUSB_ERROR_OTHER;
		}
	}
	RETURN(ret, int);
}

static void free_iso_urbs(struct android_transfer_priv *tpriv) {
	int i;
	for (i = 0; i < tpriv->num_urbs; i++) {
		struct usbfs_urb *urb = tpriv->iso_urbs[i];
		if (UNLIKELY(!urb))
			break;
		free(urb);
	}

	free(tpriv->iso_urbs);
	tpriv->iso_urbs = NULL;
}

static int submit_bulk_transfer(struct usbi_transfer *itransfer) {

	struct libusb_transfer *transfer
		= USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);
	struct android_device_handle_priv *dpriv
		= _device_handle_priv(transfer->dev_handle);
	struct usbfs_urb *urbs;
	int is_out = (transfer->endpoint & LIBUSB_ENDPOINT_DIR_MASK)
		== LIBUSB_ENDPOINT_OUT;
	int bulk_buffer_len, use_bulk_continuation;
	int r;
	int i;
	size_t alloc_size;

	if (UNLIKELY(tpriv->urbs))
		return LIBUSB_ERROR_BUSY;

	if (UNLIKELY(is_out && (transfer->flags & LIBUSB_TRANSFER_ADD_ZERO_PACKET))
			&& !(dpriv->caps & USBFS_CAP_ZERO_PACKET))
		return LIBUSB_ERROR_NOT_SUPPORTED;

	
	if (dpriv->caps & USBFS_CAP_BULK_SCATTER_GATHER) {
		
		bulk_buffer_len = transfer->length ? transfer->length : 1;
		use_bulk_continuation = 0;
	} else if (dpriv->caps & USBFS_CAP_BULK_CONTINUATION) {
		
		bulk_buffer_len = MAX_BULK_BUFFER_LENGTH;
		use_bulk_continuation = 1;
	} else if (dpriv->caps & USBFS_CAP_NO_PACKET_SIZE_LIM) {
		
		bulk_buffer_len = transfer->length ? transfer->length : 1;
		use_bulk_continuation = 0;
	} else {
		
		
		bulk_buffer_len = MAX_BULK_BUFFER_LENGTH;
		use_bulk_continuation = 0;
	}

	int num_urbs = transfer->length / bulk_buffer_len;
	int last_urb_partial = 0;

	if (transfer->length == 0) {
		num_urbs = 1;
	} else if ((transfer->length % bulk_buffer_len) > 0) {
		last_urb_partial = 1;
		num_urbs++;
	}
	usbi_dbg("need %d urbs for new transfer with length %d", num_urbs, transfer->length);
	alloc_size = num_urbs * sizeof(struct usbfs_urb);
	urbs = calloc(1, alloc_size);
	if (UNLIKELY(!urbs))
		return LIBUSB_ERROR_NO_MEM;
	tpriv->urbs = urbs;
	tpriv->num_urbs = num_urbs;
	tpriv->num_retired = 0;
	tpriv->reap_action = NORMAL;
	tpriv->reap_status = LIBUSB_TRANSFER_COMPLETED;

	for (i = 0; i < num_urbs; i++) {
		struct usbfs_urb *urb = &urbs[i];
		urb->usercontext = itransfer;
		switch (transfer->type) {
		case LIBUSB_TRANSFER_TYPE_BULK:
			urb->type = USBFS_URB_TYPE_BULK;
			urb->stream_id = 0;
			break;
		case LIBUSB_TRANSFER_TYPE_BULK_STREAM:
			urb->type = USBFS_URB_TYPE_BULK;
			urb->stream_id = itransfer->stream_id;
			break;
		case LIBUSB_TRANSFER_TYPE_INTERRUPT:
			urb->type = USBFS_URB_TYPE_INTERRUPT;
			break;
		}
		urb->endpoint = transfer->endpoint;
		urb->buffer = transfer->buffer + (i * bulk_buffer_len);
		
		if (use_bulk_continuation && !is_out && (i < num_urbs - 1))
			urb->flags = USBFS_URB_SHORT_NOT_OK;
		if (i == num_urbs - 1 && last_urb_partial)
			urb->buffer_length = transfer->length % bulk_buffer_len;
		else if (transfer->length == 0)
			urb->buffer_length = 0;
		else
			urb->buffer_length = bulk_buffer_len;

		if (i > 0 && use_bulk_continuation)
			urb->flags |= USBFS_URB_BULK_CONTINUATION;

		
		if (is_out && i == num_urbs - 1
				&& transfer->flags & LIBUSB_TRANSFER_ADD_ZERO_PACKET)
			urb->flags |= USBFS_URB_ZERO_PACKET;
#if LOCAL_DEBUG
		dump_urb(i, dpriv->fd, urb);
#endif
		r = ioctl(dpriv->fd, IOCTL_USBFS_SUBMITURB, urb);
		if (UNLIKELY(r < 0)) {
			if (errno == ENODEV) {
				r = LIBUSB_ERROR_NO_DEVICE;
			} else {
				usbi_err(TRANSFER_CTX(transfer),
					"submiturb failed error %d errno=%d", r, errno);
				r = LIBUSB_ERROR_IO;
			}

			
			if (UNLIKELY(i == 0)) {
				usbi_dbg("first URB failed, easy peasy");
				free(urbs);
				tpriv->urbs = NULL;
				return r;
			}

			
			tpriv->reap_action =
					EREMOTEIO == errno ? COMPLETED_EARLY : SUBMIT_FAILED;

			
			tpriv->num_retired += num_urbs - i;

			
			if (COMPLETED_EARLY == tpriv->reap_action)
				return LIBUSB_SUCCESS;

			discard_urbs(itransfer, 0, i);

			usbi_dbg("reporting successful submission but waiting for %d "
				"discards before reporting error", i);
			return LIBUSB_SUCCESS;
		}
	}

	return LIBUSB_SUCCESS;
}

static int submit_iso_transfer(struct usbi_transfer *itransfer) {
	struct libusb_transfer *transfer = USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);
	struct android_device_handle_priv *dpriv = _device_handle_priv(transfer->dev_handle);
	struct usbfs_urb **urbs;
	size_t alloc_size;
	const int num_packets = transfer->num_iso_packets;
	int i;
	int this_urb_len = 0;
	int num_urbs = 1;
	int packet_offset = 0;
	unsigned int packet_len;
	unsigned char *urb_buffer = transfer->buffer;

	if (UNLIKELY(tpriv->iso_urbs))
		return LIBUSB_ERROR_BUSY;

	

	
	for (i = 0; i < num_packets; i++) {
		unsigned int space_remaining = MAX_ISO_BUFFER_LENGTH - this_urb_len;
		packet_len = transfer->iso_packet_desc[i].length;

		if (packet_len > space_remaining) {
			num_urbs++;
			this_urb_len = packet_len;
		} else {
			this_urb_len += packet_len;
		}
	}
	usbi_dbg("need %d of 32k URBs for transfer", num_urbs);

	alloc_size = num_urbs * sizeof(*urbs);
	urbs = calloc(1, alloc_size);
	if (UNLIKELY(!urbs))
		return LIBUSB_ERROR_NO_MEM;

	tpriv->iso_urbs = urbs;
	tpriv->num_urbs = num_urbs;
	tpriv->num_retired = 0;
	tpriv->reap_action = NORMAL;
	tpriv->iso_packet_offset = 0;

	
	for (i = 0; i < num_urbs; i++) {
		struct usbfs_urb *urb;
		unsigned int space_remaining_in_urb = MAX_ISO_BUFFER_LENGTH;
		int urb_packet_offset = 0;
		unsigned char *urb_buffer_orig = urb_buffer;
		int j;
		int k;

		
		while (packet_offset < num_packets) {
			packet_len = transfer->iso_packet_desc[packet_offset].length;
			if (packet_len <= space_remaining_in_urb) {
				
				urb_packet_offset++;
				packet_offset++;
				space_remaining_in_urb -= packet_len;
				urb_buffer += packet_len;
			} else {
				
				break;
			}
		}

		alloc_size = sizeof(*urb)
			+ (urb_packet_offset * sizeof(struct usbfs_iso_packet_desc));
		urb = calloc(1, alloc_size);
		if (UNLIKELY(!urb)) {
			free_iso_urbs(tpriv);
			return LIBUSB_ERROR_NO_MEM;
		}
		urbs[i] = urb;

		
		for (j = 0, k = packet_offset - urb_packet_offset;
				k < packet_offset; k++, j++) {
			packet_len = transfer->iso_packet_desc[k].length;
			urb->iso_frame_desc[j].length = packet_len;
		}

		urb->usercontext = itransfer;
		urb->type = USBFS_URB_TYPE_ISO;
		
		urb->flags = USBFS_URB_ISO_ASAP;
		urb->endpoint = transfer->endpoint;
		urb->number_of_packets = urb_packet_offset;
		urb->buffer = urb_buffer_orig;
	}

	
	for (i = 0; i < num_urbs; i++) {
		int r = ioctl(dpriv->fd, IOCTL_USBFS_SUBMITURB, urbs[i]);
		if (UNLIKELY(r < 0)) {
			if (errno == ENODEV) {
				r = LIBUSB_ERROR_NO_DEVICE;
			} else {
				usbi_err(TRANSFER_CTX(transfer),
					"submiturb failed error %d errno=%d", r, errno);
				r = LIBUSB_ERROR_IO;
			}

			
			if (UNLIKELY(i == 0)) {
				usbi_dbg("first URB failed, easy peasy");
				free_iso_urbs(tpriv);
				return r;
			}

			
			tpriv->reap_action = SUBMIT_FAILED;

			
			tpriv->num_retired = num_urbs - i;
			discard_urbs(itransfer, 0, i);

			usbi_dbg("reporting successful submission but waiting for %d "
				"discards before reporting error", i);
			return LIBUSB_SUCCESS;
		}
	}

	return LIBUSB_SUCCESS;
}

static int submit_control_transfer(struct usbi_transfer *itransfer) {
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);
	struct libusb_transfer *transfer
		= USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	struct android_device_handle_priv *dpriv
		= _device_handle_priv(transfer->dev_handle);
	struct usbfs_urb *urb;
	int r;

	if (UNLIKELY(tpriv->urbs))
		return LIBUSB_ERROR_BUSY;

	if (UNLIKELY(transfer->length - LIBUSB_CONTROL_SETUP_SIZE > MAX_CTRL_BUFFER_LENGTH))
		return LIBUSB_ERROR_INVALID_PARAM;

	urb = calloc(1, sizeof(struct usbfs_urb));
	if (UNLIKELY(!urb))
		return LIBUSB_ERROR_NO_MEM;
	tpriv->urbs = urb;
	tpriv->num_urbs = 1;
	tpriv->reap_action = NORMAL;

	urb->usercontext = itransfer;
	urb->type = USBFS_URB_TYPE_CONTROL;
	urb->endpoint = transfer->endpoint;
	urb->buffer = transfer->buffer;
	urb->buffer_length = transfer->length;

	r = ioctl(dpriv->fd, IOCTL_USBFS_SUBMITURB, urb);
	if (UNLIKELY(r < 0)) {
		free(urb);
		tpriv->urbs = NULL;
		if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(TRANSFER_CTX(transfer),
			"submiturb failed error %d errno=%d", r, errno);
		return LIBUSB_ERROR_IO;
	}
	return LIBUSB_SUCCESS;
}

static int op_submit_transfer(struct usbi_transfer *itransfer) {
	struct libusb_transfer *transfer
		= USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);

	switch (transfer->type) {
	case LIBUSB_TRANSFER_TYPE_CONTROL:
		return submit_control_transfer(itransfer);
	case LIBUSB_TRANSFER_TYPE_BULK:
	case LIBUSB_TRANSFER_TYPE_BULK_STREAM:
		return submit_bulk_transfer(itransfer);
	case LIBUSB_TRANSFER_TYPE_INTERRUPT:
		return submit_bulk_transfer(itransfer);
	case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
		return submit_iso_transfer(itransfer);
	default:
		usbi_err(TRANSFER_CTX(transfer),
			"unknown endpoint type %d", transfer->type);
		return LIBUSB_ERROR_INVALID_PARAM;
	}
}

static int op_cancel_transfer(struct usbi_transfer *itransfer) {
	ENTER();
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);
	struct libusb_transfer *transfer = USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);

	switch (transfer->type) {
	case LIBUSB_TRANSFER_TYPE_BULK:
	case LIBUSB_TRANSFER_TYPE_BULK_STREAM:
		if (tpriv->reap_action == ERROR)
			break;
		
	case LIBUSB_TRANSFER_TYPE_CONTROL:
	case LIBUSB_TRANSFER_TYPE_INTERRUPT:
	case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
		tpriv->reap_action = CANCELLED;
		break;
	default:
		usbi_err(TRANSFER_CTX(transfer),
			"unknown endpoint type %d", transfer->type);
		RETURN(LIBUSB_ERROR_INVALID_PARAM, int);
	}

	if (UNLIKELY(!tpriv->urbs))
		RETURN(LIBUSB_ERROR_NOT_FOUND, int);

	RETURN(discard_urbs(itransfer, 0, tpriv->num_urbs), int);
}

static void op_clear_transfer_priv(struct usbi_transfer *itransfer) {
	struct libusb_transfer *transfer = USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);

	
	switch (transfer->type) {
	case LIBUSB_TRANSFER_TYPE_CONTROL:
	case LIBUSB_TRANSFER_TYPE_BULK:
	case LIBUSB_TRANSFER_TYPE_BULK_STREAM:
	case LIBUSB_TRANSFER_TYPE_INTERRUPT:
		usbi_mutex_lock(&itransfer->lock);
		if (tpriv->urbs)
			free(tpriv->urbs);
		tpriv->urbs = NULL;
		usbi_mutex_unlock(&itransfer->lock);
		break;
	case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
		usbi_mutex_lock(&itransfer->lock);
		if (tpriv->iso_urbs)
			free_iso_urbs(tpriv);
		usbi_mutex_unlock(&itransfer->lock);
		break;
	default:
		usbi_err(TRANSFER_CTX(transfer),
			"unknown endpoint type %d", transfer->type);
	}
}

static int handle_bulk_completion(struct libusb_device_handle *handle,	
		struct usbi_transfer *itransfer,
		struct usbfs_urb *urb) {
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);
	struct libusb_transfer *transfer = USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	int urb_idx = urb - tpriv->urbs;

	usbi_mutex_lock(&itransfer->lock);
	usbi_dbg("handling completion status %d of bulk urb %d/%d",
			urb->status, urb_idx + 1, tpriv->num_urbs);

	tpriv->num_retired++;

	if (UNLIKELY(tpriv->reap_action != NORMAL)) {
		
		usbi_dbg("abnormal reap: urb status %d", urb->status);

		
		if (urb->actual_length > 0) {
			unsigned char *target = transfer->buffer + itransfer->transferred;
			usbi_dbg("received %d bytes of surplus data", urb->actual_length);
			if (urb->buffer != target) {
				usbi_dbg("moving surplus data from offset %d to offset %d",
					(unsigned char *) urb->buffer - transfer->buffer,
					target - transfer->buffer);
				memmove(target, urb->buffer, urb->actual_length);
			}
			itransfer->transferred += urb->actual_length;
		}

		if (tpriv->num_retired == tpriv->num_urbs) {
			usbi_dbg("abnormal reap: last URB handled, reporting");
			if (tpriv->reap_action != COMPLETED_EARLY
					&& tpriv->reap_status == LIBUSB_TRANSFER_COMPLETED)
				tpriv->reap_status = LIBUSB_TRANSFER_ERROR;
			goto completed;
		}
		goto out_unlock;
	}

	itransfer->transferred += urb->actual_length;

	
	switch (urb->status) {
	case 0:
		break;
	case -EREMOTEIO: 
		break;
	case -ENOENT: 
	case -ECONNRESET:
		break;
	case -ENODEV:
	case -ESHUTDOWN:
		usbi_dbg("device removed");
		tpriv->reap_status = LIBUSB_TRANSFER_NO_DEVICE;
		goto cancel_remaining;
	case -EPIPE:
		usbi_dbg("detected endpoint stall");
		if (tpriv->reap_status == LIBUSB_TRANSFER_COMPLETED)
			tpriv->reap_status = LIBUSB_TRANSFER_STALL;
		LOGE("LIBUSB_TRANSFER_STALL");
		op_clear_halt(handle, urb->endpoint);	
		goto cancel_remaining;
	case -EOVERFLOW:
		
		usbi_dbg("overflow, actual_length=%d", urb->actual_length);
		if (tpriv->reap_status == LIBUSB_TRANSFER_COMPLETED)
			tpriv->reap_status = LIBUSB_TRANSFER_OVERFLOW;
		goto completed;
	case -ETIME:
	case -EPROTO:
	case -EILSEQ:
	case -ECOMM:
	case -ENOSR:
		usbi_dbg("low level error %d", urb->status);
		tpriv->reap_action = ERROR;
		goto cancel_remaining;
	default:
		usbi_warn(ITRANSFER_CTX(itransfer),
			"unrecognised urb status %d", urb->status);
		tpriv->reap_action = ERROR;
		goto cancel_remaining;
	}

	
	if (urb_idx == tpriv->num_urbs - 1) {
		usbi_dbg("last URB in transfer --> complete!");
		goto completed;
	} else if (urb->actual_length < urb->buffer_length) {
		usbi_dbg("short transfer %d/%d --> complete!",
			urb->actual_length, urb->buffer_length);
		if (tpriv->reap_action == NORMAL)
			tpriv->reap_action = COMPLETED_EARLY;
	} else
		goto out_unlock;

cancel_remaining:
	if (ERROR == tpriv->reap_action
			&& LIBUSB_TRANSFER_COMPLETED == tpriv->reap_status)
		tpriv->reap_status = LIBUSB_TRANSFER_ERROR;

	if (tpriv->num_retired == tpriv->num_urbs) 
		goto completed;

	
	discard_urbs(itransfer, urb_idx + 1, tpriv->num_urbs);

out_unlock:
	usbi_mutex_unlock(&itransfer->lock);
	return LIBUSB_SUCCESS;

completed:
	if (tpriv->urbs)
		free(tpriv->urbs);
	tpriv->urbs = NULL;
	usbi_mutex_unlock(&itransfer->lock);
	return CANCELLED == tpriv->reap_action ?
		usbi_handle_transfer_cancellation(itransfer) :
		usbi_handle_transfer_completion(itransfer, tpriv->reap_status);
}

static int handle_iso_completion(struct libusb_device_handle *handle,	
		struct usbi_transfer *itransfer,
		struct usbfs_urb *urb) {
	struct libusb_transfer *transfer = USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);
	int num_urbs = tpriv->num_urbs;
	int urb_idx = 0;
	int i;
	enum libusb_transfer_status status = LIBUSB_TRANSFER_COMPLETED;

	usbi_mutex_lock(&itransfer->lock);
	for (i = 0; i < num_urbs; i++) {
	    
	    
	    if (tpriv->iso_urbs == NULL) {
			break;
	    }
		if (urb == tpriv->iso_urbs[i]) {
			urb_idx = i + 1;
			break;
		}
	}
	if (UNLIKELY(urb_idx == 0)) {
		usbi_err(TRANSFER_CTX(transfer), "could not locate urb!");	
		usbi_mutex_unlock(&itransfer->lock);
		return LIBUSB_ERROR_NOT_FOUND;
	}

	usbi_dbg("handling completion status %d of iso urb %d/%d",
		urb->status, urb_idx, num_urbs);

	

	for (i = 0; i < urb->number_of_packets; i++) {
		struct usbfs_iso_packet_desc *urb_desc = &urb->iso_frame_desc[i];
		struct libusb_iso_packet_descriptor *lib_desc =
				&transfer->iso_packet_desc[tpriv->iso_packet_offset++];
		lib_desc->status = LIBUSB_TRANSFER_COMPLETED;
		switch (urb_desc->status) {
		case 0:
			break;
		case -ENOENT: 
		case -ECONNRESET:
			break;
		case -ENODEV:
		case -ESHUTDOWN:
			usbi_dbg("device removed");
			lib_desc->status = LIBUSB_TRANSFER_NO_DEVICE;
			break;
		case -EPIPE:
			usbi_dbg("detected endpoint stall");
			lib_desc->status = LIBUSB_TRANSFER_STALL;
			LOGE("LIBUSB_TRANSFER_STALL");
			op_clear_halt(handle, urb->endpoint);	
			break;
		case -EOVERFLOW:
			usbi_dbg("overflow error");
			lib_desc->status = LIBUSB_TRANSFER_OVERFLOW;
			break;
		case -ETIME:
		case -EPROTO:
		case -EILSEQ:
		case -ECOMM:
		case -ENOSR:
		case -EXDEV:
			usbi_dbg("low-level USB error %d", urb_desc->status);
			lib_desc->status = LIBUSB_TRANSFER_ERROR;
			break;
		default:
			usbi_warn(TRANSFER_CTX(transfer),
				"unrecognised urb status %d", urb_desc->status);
			lib_desc->status = LIBUSB_TRANSFER_ERROR;
			break;
		}
		lib_desc->actual_length = urb_desc->actual_length;
	}

	tpriv->num_retired++;

	if (UNLIKELY(tpriv->reap_action != NORMAL)) { 
		usbi_dbg("CANCEL: urb status %d", urb->status);

		if (tpriv->num_retired == num_urbs) {
			usbi_dbg("CANCEL: last URB handled, reporting");
			free_iso_urbs(tpriv);
			if (tpriv->reap_action == CANCELLED) {
				usbi_mutex_unlock(&itransfer->lock);
				return usbi_handle_transfer_cancellation(itransfer);
			} else {
				usbi_mutex_unlock(&itransfer->lock);
				return usbi_handle_transfer_completion(itransfer, LIBUSB_TRANSFER_ERROR);
			}
		}
		goto out;
	}

	switch (urb->status) {
	case 0:
		break;
	case -ENOENT: 
	case -ECONNRESET:
		break;
	case -ESHUTDOWN:
		usbi_dbg("device removed");
		status = LIBUSB_TRANSFER_NO_DEVICE;
		break;
	default:
		usbi_warn(TRANSFER_CTX(transfer),
			"unrecognised urb status %d", urb->status);
		status = LIBUSB_TRANSFER_ERROR;
		break;
	}

	
	if (urb_idx == num_urbs) {
		usbi_dbg("last URB in transfer --> complete!");
		free_iso_urbs(tpriv);
		usbi_mutex_unlock(&itransfer->lock);
		return usbi_handle_transfer_completion(itransfer, status);
	}

out:
	usbi_mutex_unlock(&itransfer->lock);
	return LIBUSB_SUCCESS;
}

static int handle_control_completion(struct libusb_device_handle *handle,	
		struct usbi_transfer *itransfer,
		struct usbfs_urb *urb) {
	struct android_transfer_priv *tpriv = usbi_transfer_get_os_priv(itransfer);
	int status;

	usbi_mutex_lock(&itransfer->lock);
	usbi_dbg("handling completion status %d", urb->status);

	itransfer->transferred += urb->actual_length;

	if (UNLIKELY(tpriv->reap_action == CANCELLED)) {
		if (urb->status != 0 && urb->status != -ENOENT)
			usbi_warn(ITRANSFER_CTX(itransfer),
				"cancel: unrecognised urb status %d", urb->status);
		if (tpriv->urbs) {
			free(tpriv->urbs);
			tpriv->urbs = NULL;
		}
		usbi_mutex_unlock(&itransfer->lock);
		return usbi_handle_transfer_cancellation(itransfer);
	}

	switch (urb->status) {
	case 0:
		status = LIBUSB_TRANSFER_COMPLETED;
		break;
	case -ENOENT: 
		status = LIBUSB_TRANSFER_CANCELLED;
		break;
	case -ENODEV:
	case -ESHUTDOWN:
		usbi_dbg("device removed");
		status = LIBUSB_TRANSFER_NO_DEVICE;
		break;
	case -EPIPE:
		usbi_dbg("unsupported control request");
		status = LIBUSB_TRANSFER_STALL;
		LOGE("LIBUSB_TRANSFER_STALL");
		op_clear_halt(handle, urb->endpoint);	
		break;
	case -EOVERFLOW:
		usbi_dbg("control overflow error");
		status = LIBUSB_TRANSFER_OVERFLOW;
		break;
	case -ETIME:
	case -EPROTO:
	case -EILSEQ:
	case -ECOMM:
	case -ENOSR:
		usbi_dbg("low-level bus error occurred");
		status = LIBUSB_TRANSFER_ERROR;
		break;
	default:
		usbi_warn(ITRANSFER_CTX(itransfer),
			"unrecognised urb status %d", urb->status);
		status = LIBUSB_TRANSFER_ERROR;
		break;
	}

	if (tpriv->urbs) {
		free(tpriv->urbs);	
		tpriv->urbs = NULL;
	}
	usbi_mutex_unlock(&itransfer->lock);
	return usbi_handle_transfer_completion(itransfer, status);
}

static int reap_for_handle(struct libusb_device_handle *handle) {
	struct android_device_handle_priv *hpriv = _device_handle_priv(handle);
	int r;
	struct usbfs_urb *urb;
	struct usbi_transfer *itransfer;
	struct libusb_transfer *transfer;

	r = ioctl(hpriv->fd, IOCTL_USBFS_REAPURBNDELAY, &urb);
	if (r == -1 && errno == EAGAIN)
		return 1;
	if (UNLIKELY(r < 0)) {
		if (errno == ENODEV)
			return LIBUSB_ERROR_NO_DEVICE;

		usbi_err(HANDLE_CTX(handle), "reap failed error %d errno=%d", r, errno);
		return LIBUSB_ERROR_IO;
	}

	itransfer = urb->usercontext;
	transfer = USBI_TRANSFER_TO_LIBUSB_TRANSFER(itransfer);

	usbi_dbg("urb type=%d status=%d transferred=%d",
		urb->type, urb->status, urb->actual_length);

	switch (transfer->type) {
	case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
		return handle_iso_completion(handle, itransfer, urb);
	case LIBUSB_TRANSFER_TYPE_BULK:
	case LIBUSB_TRANSFER_TYPE_BULK_STREAM:
	case LIBUSB_TRANSFER_TYPE_INTERRUPT:
		return handle_bulk_completion(handle, itransfer, urb);
	case LIBUSB_TRANSFER_TYPE_CONTROL:
		return handle_control_completion(handle, itransfer, urb);
	default:
		usbi_err(HANDLE_CTX(handle),
			"unrecognised endpoint type %x", transfer->type);
		return LIBUSB_ERROR_OTHER;
	}
}

static int op_handle_events(struct libusb_context *ctx, struct pollfd *fds,
		POLL_NFDS_TYPE nfds, int num_ready) {
	int r;
	unsigned int i = 0;

	usbi_mutex_lock(&ctx->open_devs_lock);
	for (i = 0; i < nfds && num_ready > 0; i++) {
		struct pollfd *pollfd = &fds[i];
		struct libusb_device_handle *handle;
		struct android_device_handle_priv *hpriv = NULL;

		if (!pollfd->revents)
			continue;

		num_ready--;
		list_for_each_entry(handle, &ctx->open_devs, list, struct libusb_device_handle)
		{
			hpriv = _device_handle_priv(handle);
			if (hpriv->fd == pollfd->fd)
				break;
		}

		if (!hpriv || hpriv->fd != pollfd->fd) {
			usbi_err(ctx, "cannot find handle for fd %d\n",
				 pollfd->fd);
			continue;
		}

		if (pollfd->revents & POLLERR) {
			usbi_remove_pollfd(HANDLE_CTX(handle), hpriv->fd);
			usbi_mutex_lock(&ctx->events_lock);		
			usbi_handle_disconnect(handle);
			usbi_mutex_unlock(&ctx->events_lock);	
			
			usbi_mutex_static_lock(&android_hotplug_lock);
			if (handle->dev->attached)
				android_device_disconnected(handle->dev->bus_number,
						handle->dev->device_address, NULL);
			usbi_mutex_static_unlock(&android_hotplug_lock);
			continue;
		}

		do {
			r = reap_for_handle(handle);
		} while (r == 0);
		if (r == 1 || r == LIBUSB_ERROR_NO_DEVICE)
			continue;
		else if (r < 0)
			goto out;
	}

	r = 0;
out:
	usbi_mutex_unlock(&ctx->open_devs_lock);
	return r;
}

static int op_clock_gettime(int clk_id, struct timespec *tp) {
	switch (clk_id) {
	case USBI_CLOCK_MONOTONIC:
		return clock_gettime(monotonic_clkid, tp);
	case USBI_CLOCK_REALTIME:
		return clock_gettime(CLOCK_REALTIME, tp);
	default:
		return LIBUSB_ERROR_INVALID_PARAM;
	}
}

#ifdef USBI_TIMERFD_AVAILABLE
static clockid_t op_get_timerfd_clockid(void)
{
	return monotonic_clkid;

}
#endif

const struct usbi_os_backend android_usbfs_backend = {
	.name = "Android usbfs",
	.caps = USBI_CAP_HAS_HID_ACCESS | USBI_CAP_SUPPORTS_DETACH_KERNEL_DRIVER,
	.init = op_init,
	.init2 = op_init2,	
	.exit = op_exit,
	.get_device_list = NULL,
	.hotplug_poll = op_hotplug_poll,
	.get_raw_descriptor = op_get_raw_descriptor,	
	.get_device_descriptor = op_get_device_descriptor,
	.get_active_config_descriptor = op_get_active_config_descriptor,
	.get_config_descriptor = op_get_config_descriptor,
	.get_config_descriptor_by_value = op_get_config_descriptor_by_value,
	.set_device_fd = op_set_device_fd,	
	.open = op_open,
	.close = op_close,
	.get_configuration = op_get_configuration,
	.set_configuration = op_set_configuration,
	.claim_interface = op_claim_interface,
	.release_interface = op_release_interface,

	.set_interface_altsetting = op_set_interface,
	.clear_halt = op_clear_halt,
	.reset_device = op_reset_device,

	.alloc_streams = op_alloc_streams,
	.free_streams = op_free_streams,

	.kernel_driver_active = op_kernel_driver_active,
	.detach_kernel_driver = op_detach_kernel_driver,
	.attach_kernel_driver = op_attach_kernel_driver,

	.destroy_device = op_destroy_device,

	.submit_transfer = op_submit_transfer,
	.cancel_transfer = op_cancel_transfer,
	.clear_transfer_priv = op_clear_transfer_priv,

	.handle_events = op_handle_events,

	.clock_gettime = op_clock_gettime,

#ifdef USBI_TIMERFD_AVAILABLE
	.get_timerfd_clockid = op_get_timerfd_clockid,
#endif

	.device_priv_size = sizeof(struct android_device_priv),
	.device_handle_priv_size = sizeof(struct android_device_handle_priv),
	.transfer_priv_size = sizeof(struct android_transfer_priv),
	.add_iso_packet_size = 0,
};
