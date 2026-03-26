




#define LOCAL_DEBUG 0

#define LOG_TAG "libuvc/device"
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

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

#define UVC_DETACH_ATTACH 0	

int uvc_already_open(uvc_context_t *ctx, struct libusb_device *usb_dev);
void uvc_free_devh(uvc_device_handle_t *devh);

uvc_error_t uvc_get_device_info(uvc_device_t *dev, uvc_device_info_t **info);
void uvc_free_device_info(uvc_device_info_t *info);

uvc_error_t uvc_scan_control(uvc_device_t *dev, uvc_device_info_t *info);
uvc_error_t uvc_parse_vc(uvc_device_t *dev, uvc_device_info_t *info,
		const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_extension_unit(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_header(uvc_device_t *dev, uvc_device_info_t *info,
		const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_input_terminal(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vc_processing_unit(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size);

uvc_error_t uvc_scan_streaming(uvc_device_t *dev, uvc_device_info_t *info,
		int interface_idx);
uvc_error_t uvc_parse_vs(uvc_device_t *dev, uvc_device_info_t *info,
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size);
uvc_error_t uvc_parse_vs_format_uncompressed(
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size);
uvc_error_t uvc_parse_vs_format_mjpeg(uvc_streaming_interface_t *stream_if,
		const unsigned char *block, size_t block_size);
uvc_error_t uvc_parse_vs_frame_uncompressed(
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size);
uvc_error_t uvc_parse_vs_frame_format(uvc_streaming_interface_t *stream_if,
	    const unsigned char *block,
	    size_t block_size);
uvc_error_t uvc_parse_vs_frame_frame(uvc_streaming_interface_t *stream_if,
	    const unsigned char *block,
	    size_t block_size);
uvc_error_t uvc_parse_vs_input_header(uvc_streaming_interface_t *stream_if,
		const unsigned char *block, size_t block_size);

void _uvc_status_callback(struct libusb_transfer *transfer);


int uvc_already_open(uvc_context_t *ctx, struct libusb_device *usb_dev) {
	uvc_device_handle_t *devh;

	DL_FOREACH(ctx->open_devices, devh)
	{
		if (usb_dev == devh->dev->usb_dev)
			return 1;
	}

	return 0;
}


uvc_error_t uvc_find_device(uvc_context_t *ctx, uvc_device_t **dev, int vid,
		int pid, const char *sn) {
	uvc_error_t ret = UVC_SUCCESS;

	uvc_device_t **list;
	uvc_device_t *test_dev;
	int dev_idx;
	int found_dev;

	UVC_ENTER();

	ret = uvc_get_device_list(ctx, &list);

	if (UNLIKELY(ret != UVC_SUCCESS)) {
		UVC_EXIT(ret);
		return ret;
	}

	dev_idx = 0;
	found_dev = 0;

	while (!found_dev && (test_dev = list[dev_idx++]) != NULL ) {
		uvc_device_descriptor_t *desc;

		if (uvc_get_device_descriptor(test_dev, &desc) != UVC_SUCCESS)
			continue;

		if ((!vid || desc->idVendor == vid) && (!pid || desc->idProduct == pid)
				&& (!sn || (desc->serialNumber && !strcmp(desc->serialNumber, sn))))
			found_dev = 1;

		uvc_free_device_descriptor(desc);
	}

	if (found_dev)
		uvc_ref_device(test_dev);

	uvc_free_device_list(list, 1);

	if (found_dev) {
		*dev = test_dev;
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS;
	} else {
		UVC_EXIT(UVC_ERROR_NO_DEVICE);
		return UVC_ERROR_NO_DEVICE;
	}
}


uvc_error_t uvc_find_device2(uvc_context_t *ctx, uvc_device_t **device, int vid,
		int pid, const char *sn, int fd) {

	UVC_ENTER();

	LOGD("call libusb_find_device");
	struct libusb_device *usb_dev = libusb_find_device(ctx->usb_ctx, vid, pid, sn, fd);

	if (usb_dev) {
		*device = malloc(sizeof(uvc_device_t));
		(*device)->ctx = ctx;
		(*device)->ref = 0;
		(*device)->usb_dev = usb_dev;
		libusb_set_device_fd(usb_dev, fd);	
		uvc_ref_device(*device);
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS;
	} else {
		LOGE("could not find specific device");
		*device = NULL;
	}
	UVC_EXIT(UVC_ERROR_NO_DEVICE);
	return UVC_ERROR_NO_DEVICE;
}


uvc_error_t uvc_get_device_with_fd(uvc_context_t *ctx, uvc_device_t **device,
		int vid, int pid, const char *serial, int fd, int busnum, int devaddr) {

	ENTER();

	LOGD("call libusb_get_device_with_fd");
	struct libusb_device *usb_dev = libusb_get_device_with_fd(ctx->usb_ctx, vid, pid, serial, fd, busnum, devaddr);

	if (LIKELY(usb_dev)) {
		*device = malloc(sizeof(uvc_device_t));
		(*device)->ctx = ctx;
		(*device)->ref = 0;
		(*device)->usb_dev = usb_dev;

		uvc_ref_device(*device);
		UVC_EXIT(UVC_SUCCESS);
		RETURN(UVC_SUCCESS, int);
	} else {
		LOGE("could not find specific device");
		*device = NULL;
		RETURN(UVC_ERROR_NO_DEVICE, int);
	}

}


uint8_t uvc_get_bus_number(uvc_device_t *dev) {
	return libusb_get_bus_number(dev->usb_dev);
}


uint8_t uvc_get_device_address(uvc_device_t *dev) {
	return libusb_get_device_address(dev->usb_dev);
}


uvc_error_t uvc_open(uvc_device_t *dev, uvc_device_handle_t **devh) {
	uvc_error_t ret;
	struct libusb_device_handle *usb_devh;
	uvc_device_handle_t *internal_devh;
	struct libusb_device_descriptor desc;

	UVC_ENTER();

	ret = libusb_open(dev->usb_dev, &usb_devh);
	UVC_DEBUG("libusb_open() = %d", ret);

	if (UNLIKELY(ret != UVC_SUCCESS)) {
		UVC_EXIT(ret);
		return ret;
	}

	uvc_ref_device(dev);

	internal_devh = calloc(1, sizeof(*internal_devh));
	internal_devh->dev = dev;
	internal_devh->usb_devh = usb_devh;
	internal_devh->reset_on_release_if = 0;	
	ret = uvc_get_device_info(dev, &(internal_devh->info));
	pthread_mutex_init(&internal_devh->status_mutex, NULL);	

	if (UNLIKELY(ret != UVC_SUCCESS))
		goto fail2;	
#if !UVC_DETACH_ATTACH
	
	libusb_set_auto_detach_kernel_driver(usb_devh, 1);
#endif
	UVC_DEBUG("claiming control interface %d",
			internal_devh->info->ctrl_if.bInterfaceNumber);
	ret = uvc_claim_if(internal_devh,
			internal_devh->info->ctrl_if.bInterfaceNumber);
	if (UNLIKELY(ret != UVC_SUCCESS))
		goto fail;

	libusb_get_device_descriptor(dev->usb_dev, &desc);
	internal_devh->is_isight = (desc.idVendor == 0x05ac && desc.idProduct == 0x8501);

	if (internal_devh->info->ctrl_if.bEndpointAddress) {
		UVC_DEBUG("status check transfer:bEndpointAddress=0x%02x", internal_devh->info->ctrl_if.bEndpointAddress);
		internal_devh->status_xfer = libusb_alloc_transfer(0);
		if (UNLIKELY(!internal_devh->status_xfer)) {
			ret = UVC_ERROR_NO_MEM;
			goto fail;
		}

		libusb_fill_interrupt_transfer(internal_devh->status_xfer, usb_devh,
				internal_devh->info->ctrl_if.bEndpointAddress,
				internal_devh->status_buf, sizeof(internal_devh->status_buf),
				_uvc_status_callback, internal_devh, 0);
		ret = libusb_submit_transfer(internal_devh->status_xfer);
		UVC_DEBUG("libusb_submit_transfer() = %d", ret);

		if (UNLIKELY(ret)) {
			LOGE("device has a status interrupt endpoint, but unable to read from it");
			goto fail;
		}
	} else {
		LOGE("internal_devh->info->ctrl_if.bEndpointAddress is null");
	}

	if (dev->ctx->own_usb_ctx && dev->ctx->open_devices == NULL) {
		
		uvc_start_handler_thread(dev->ctx);
	}

	DL_APPEND(dev->ctx->open_devices, internal_devh);
	*devh = internal_devh;

	UVC_EXIT(ret);

	return ret;

fail:
	uvc_release_if(internal_devh, internal_devh->info->ctrl_if.bInterfaceNumber);	
fail2:
#if !UVC_DETACH_ATTACH
	
	libusb_set_auto_detach_kernel_driver(usb_devh, 0);
#endif
	libusb_close(usb_devh);
	uvc_unref_device(dev);
	uvc_free_devh(internal_devh);

	UVC_EXIT(ret);

	return ret;
}


uvc_error_t uvc_get_device_info(uvc_device_t *dev, uvc_device_info_t **info) {
	uvc_error_t ret;
	uvc_device_info_t *internal_info;

	UVC_ENTER();

	internal_info = calloc(1, sizeof(*internal_info));
	if (!internal_info) {
		UVC_EXIT(UVC_ERROR_NO_MEM);
		return UVC_ERROR_NO_MEM;
	}
	if (libusb_get_config_descriptor(dev->usb_dev, 0, &(internal_info->config)) != 0) {

		
		
		
		free(internal_info);
		UVC_EXIT(UVC_ERROR_IO);
		return UVC_ERROR_IO;
	}

	ret = uvc_scan_control(dev, internal_info);
	if (UNLIKELY(ret)) {
		uvc_free_device_info(internal_info);
		UVC_EXIT(ret);
		return ret;
	}

	*info = internal_info;

	UVC_EXIT(ret);
	return ret;
}


void uvc_free_device_info(uvc_device_info_t *info) {
	uvc_input_terminal_t *input_term, *input_term_tmp;
	uvc_output_terminal_t *output_term, *output_term_tmp;
	uvc_processing_unit_t *proc_unit, *proc_unit_tmp;
	uvc_extension_unit_t *ext_unit, *ext_unit_tmp;

	uvc_streaming_interface_t *stream_if, *stream_if_tmp;
	uvc_format_desc_t *format, *format_tmp;
	uvc_frame_desc_t *frame, *frame_tmp;

	UVC_ENTER();

	DL_FOREACH_SAFE(info->ctrl_if.input_term_descs, input_term, input_term_tmp)
	{
		DL_DELETE(info->ctrl_if.input_term_descs, input_term);
		free(input_term);
	}

	DL_FOREACH_SAFE(info->ctrl_if.output_term_descs, output_term, output_term_tmp)
	{
		DL_DELETE(info->ctrl_if.output_term_descs, output_term);
		free(output_term);
	}

	DL_FOREACH_SAFE(info->ctrl_if.processing_unit_descs, proc_unit, proc_unit_tmp)
	{
		DL_DELETE(info->ctrl_if.processing_unit_descs, proc_unit);
		free(proc_unit);
	}

	DL_FOREACH_SAFE(info->ctrl_if.extension_unit_descs, ext_unit, ext_unit_tmp)
	{
		DL_DELETE(info->ctrl_if.extension_unit_descs, ext_unit);
		free(ext_unit);
	}

	DL_FOREACH_SAFE(info->stream_ifs, stream_if, stream_if_tmp)
	{
		DL_FOREACH_SAFE(stream_if->format_descs, format, format_tmp)
		{
			DL_FOREACH_SAFE(format->frame_descs, frame, frame_tmp)
			{
				if (frame->intervals)
					free(frame->intervals);

				DL_DELETE(format->frame_descs, frame);
				free(frame);
			}

			DL_DELETE(stream_if->format_descs, format);
			free(format);
		}

		DL_DELETE(info->stream_ifs, stream_if);
		free(stream_if->bmaControls);	
		free(stream_if);
	}

	if (info->config)
		libusb_free_config_descriptor(info->config);

	free(info);

	UVC_EXIT_VOID();
}


uvc_error_t uvc_get_device_descriptor(uvc_device_t *dev,
		uvc_device_descriptor_t **desc) {

	uvc_device_descriptor_t *desc_internal;
	struct libusb_device_descriptor usb_desc;
	struct libusb_device_handle *usb_devh;
	uvc_error_t ret;

	UVC_ENTER();

	ret = libusb_get_device_descriptor(dev->usb_dev, &usb_desc);

	if (UNLIKELY(ret)) {
		UVC_EXIT(ret);
		return ret;
	}

	desc_internal = calloc(1, sizeof(*desc_internal));
	desc_internal->idVendor = usb_desc.idVendor;
	desc_internal->idProduct = usb_desc.idProduct;

	if (libusb_open(dev->usb_dev, &usb_devh) == 0) {
		unsigned char str_buf[255];	
		
		int str_bytes = libusb_get_string_descriptor_ascii(usb_devh,
			usb_desc.iSerialNumber, str_buf, sizeof(str_buf));

		if (str_bytes > 0)
			desc_internal->serialNumber = strdup((const char*) str_buf);

		
		str_bytes = libusb_get_string_descriptor_ascii(usb_devh,
			usb_desc.iManufacturer, str_buf, sizeof(str_buf));

		if (str_bytes > 0)
			desc_internal->manufacturer = strdup((const char*) str_buf);

		
		str_bytes = libusb_get_string_descriptor_ascii(usb_devh,
			usb_desc.iProduct, str_buf, sizeof(str_buf));

		if (str_bytes > 0)
			desc_internal->product = strdup((const char*) str_buf);

		libusb_close(usb_devh);
	} else {
		UVC_DEBUG("can't open device %04x:%04x, not fetching serial",
				usb_desc.idVendor, usb_desc.idProduct);
	}

	*desc = desc_internal;

	UVC_EXIT(ret);
	return ret;
}


void uvc_free_device_descriptor(uvc_device_descriptor_t *desc) {
	UVC_ENTER();

	if (desc->serialNumber)
		free((void*) desc->serialNumber);

	if (desc->manufacturer)
		free((void*) desc->manufacturer);

	if (desc->product)
		free((void*) desc->product);

	free(desc);

	UVC_EXIT_VOID();
}


uvc_error_t uvc_get_device_list(uvc_context_t *ctx, uvc_device_t ***list) {
	uvc_error_t ret;
	struct libusb_device **usb_dev_list;
	struct libusb_device *usb_dev;
	int num_usb_devices;

	uvc_device_t **list_internal;
	int num_uvc_devices;

	
	int dev_idx;
	struct libusb_device_handle *usb_devh;
	struct libusb_config_descriptor *config;
	struct libusb_device_descriptor desc;
	uint8_t got_interface;

	
	int interface_idx;
	const struct libusb_interface *interface;

	
	int altsetting_idx;
	const struct libusb_interface_descriptor *if_desc;

	UVC_ENTER();

	num_usb_devices = libusb_get_device_list(ctx->usb_ctx, &usb_dev_list);

	if (UNLIKELY(num_usb_devices < 0)) {
		UVC_EXIT(UVC_ERROR_IO);
		return UVC_ERROR_IO;
	}

	list_internal = malloc(sizeof(*list_internal));
	*list_internal = NULL;

	num_uvc_devices = 0;
	dev_idx = -1;

	while ((usb_dev = usb_dev_list[++dev_idx]) != NULL ) {
		usb_devh = NULL;
		got_interface = 0;

		if (libusb_get_config_descriptor(usb_dev, 0, &config) != 0)
			continue;

		if (libusb_get_device_descriptor (usb_dev, &desc) != LIBUSB_SUCCESS)
			continue;

		
		if ((0x199e == desc.idVendor) && (0x8101 == desc.idProduct)) {
			got_interface = 1;
		} else {
			for (interface_idx = 0;
					!got_interface && interface_idx < config->bNumInterfaces;
					++interface_idx) {
				interface = &config->interface[interface_idx];

				for (altsetting_idx = 0;
						!got_interface && altsetting_idx < interface->num_altsetting;
						++altsetting_idx) {
					if_desc = &interface->altsetting[altsetting_idx];

					
					if (if_desc->bInterfaceClass == 14
							&& if_desc->bInterfaceSubClass == 2) {
						got_interface = 1;
					}
				}
			}
		}

		libusb_free_config_descriptor(config);

		if (got_interface) {
			uvc_device_t *uvc_dev = malloc(sizeof(*uvc_dev));
			uvc_dev->ctx = ctx;
			uvc_dev->ref = 0;
			uvc_dev->usb_dev = usb_dev;
			uvc_ref_device(uvc_dev);

			num_uvc_devices++;
			list_internal = realloc(list_internal,
					(num_uvc_devices + 1) * sizeof(*list_internal));

			list_internal[num_uvc_devices - 1] = uvc_dev;
			list_internal[num_uvc_devices] = NULL;

			UVC_DEBUG("    UVC: %d", dev_idx);
		} else {
			UVC_DEBUG("non-UVC: %d", dev_idx);
		}
	}

	libusb_free_device_list(usb_dev_list, 1);

	*list = list_internal;

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


void uvc_free_device_list(uvc_device_t **list, uint8_t unref_devices) {
	uvc_device_t *dev;
	int dev_idx = 0;

	UVC_ENTER();

	if (unref_devices) {
		while ((dev = list[dev_idx++]) != NULL ) {
			uvc_unref_device(dev);
		}
	}

	free(list);

	UVC_EXIT_VOID();
}


uvc_device_t *uvc_get_device(uvc_device_handle_t *devh) {
	uvc_ref_device(devh->dev);
	return devh->dev;
}


libusb_device_handle *uvc_get_libusb_handle(uvc_device_handle_t *devh) {
	return devh->usb_devh;
}


const uvc_input_terminal_t *uvc_get_input_terminals(uvc_device_handle_t *devh) {
	return devh->info->ctrl_if.input_term_descs;
}


const uvc_output_terminal_t *uvc_get_output_terminals(uvc_device_handle_t *devh) {
	return devh->info->ctrl_if.output_term_descs ;
}


const uvc_processing_unit_t *uvc_get_processing_units(uvc_device_handle_t *devh) {
	return devh->info->ctrl_if.processing_unit_descs;
}


const uvc_extension_unit_t *uvc_get_extension_units(uvc_device_handle_t *devh) {
	return devh->info->ctrl_if.extension_unit_descs;
}


void uvc_ref_device(uvc_device_t *dev) {
	UVC_ENTER();

	dev->ref++;	

	libusb_ref_device(dev->usb_dev);

	UVC_EXIT_VOID();
}


void uvc_unref_device(uvc_device_t *dev) {
	UVC_ENTER();

	libusb_unref_device(dev->usb_dev);
	dev->ref--;	


	if (dev->ref == 0) {
		free(dev);
		dev = NULL;
	}

	UVC_EXIT_VOID();
}


uvc_error_t uvc_claim_if(uvc_device_handle_t *devh, int idx) {
	int ret;

	UVC_ENTER();
#if !UVC_DETACH_ATTACH
	
	UVC_DEBUG("claiming interface %d", idx);
	ret = libusb_claim_interface(devh->usb_devh, idx);
#else
	
	ret = libusb_detach_kernel_driver(devh->usb_devh, idx);
	
	if LIKELY(!ret || ret == LIBUSB_ERROR_NOT_FOUND || ret == LIBUSB_ERROR_NOT_SUPPORTED) {
		UVC_DEBUG("claiming interface %d", idx);
		ret = libusb_claim_interface(devh->usb_devh, idx);
	} else {
	    UVC_DEBUG("not claiming interface %d: unable to detach kernel driver (%s)",
			idx, uvc_strerror(ret));
	}
#endif
	UVC_EXIT(ret);
	return ret;
}


uvc_error_t uvc_release_if(uvc_device_handle_t *devh, int idx) {
	int ret;

	UVC_ENTER();
	UVC_DEBUG("releasing interface %d", idx);
	
	
	
	
	if (devh->reset_on_release_if)
		libusb_set_interface_alt_setting(devh->usb_devh, idx, 0);

	ret = libusb_release_interface(devh->usb_devh, idx);

#if !UVC_DETACH_ATTACH
	
	
#else
	if (UVC_SUCCESS == ret) {
		
		ret = libusb_attach_kernel_driver(devh->usb_devh, idx);

		if LIKELY(!ret) {
			UVC_DEBUG("reattached kernel driver to interface %d", idx);
		} else if (ret == LIBUSB_ERROR_NOT_FOUND || ret == LIBUSB_ERROR_NOT_SUPPORTED) {
			ret = UVC_SUCCESS;  
		} else {
			UVC_DEBUG("error reattaching kernel driver to interface %d: %s",
                idx, uvc_strerror(ret));
		}
	}
#endif
	UVC_EXIT(ret);
	return ret;
}


uvc_error_t uvc_scan_control(uvc_device_t *dev, uvc_device_info_t *info) {
	const struct libusb_interface_descriptor *if_desc;
	uvc_error_t parse_ret, ret;
	int interface_idx;
	const unsigned char *buffer;
	size_t buffer_left, block_size;

	UVC_ENTER();

	ret = UVC_SUCCESS;
	if_desc = NULL;

	if (LIKELY(info && info->config)) {	
		MARK("bNumInterfaces=%d", info->config->bNumInterfaces);
		for (interface_idx = 0; interface_idx < info->config->bNumInterfaces; ++interface_idx) {
			if_desc = &info->config->interface[interface_idx].altsetting[0];
			MARK("interface_idx=%d:bInterfaceClass=%02x,bInterfaceSubClass=%02x", interface_idx, if_desc->bInterfaceClass, if_desc->bInterfaceSubClass);
			
			if (if_desc->bInterfaceClass == LIBUSB_CLASS_VIDEO && if_desc->bInterfaceSubClass == 1) 
				break;

			
			if (if_desc->bInterfaceClass == 255 && if_desc->bInterfaceSubClass == 1) {
				uvc_device_descriptor_t* dev_desc;
				int haveTISCamera = 0;
				uvc_get_device_descriptor (dev, &dev_desc);
				if (dev_desc->idVendor == 0x199e && dev_desc->idProduct == 0x8101) {
					haveTISCamera = 1;
				}
				uvc_free_device_descriptor (dev_desc);
				if (haveTISCamera) {
					break;
				}
			}
			if_desc = NULL;
		}
	}

	if (UNLIKELY(!if_desc)) {
		UVC_EXIT(UVC_ERROR_INVALID_DEVICE);
		LOGE("UVC_ERROR_INVALID_DEVICE");
		return UVC_ERROR_INVALID_DEVICE;
	}

	info->ctrl_if.bInterfaceNumber = interface_idx;
	if (if_desc->bNumEndpoints != 0) {
		info->ctrl_if.bEndpointAddress = if_desc->endpoint[0].bEndpointAddress;
	}

	buffer = if_desc->extra;
	buffer_left = if_desc->extra_length;

	while (buffer_left >= 3) { 
		block_size = buffer[0];
		parse_ret = uvc_parse_vc(dev, info, buffer, block_size);

		if (parse_ret != UVC_SUCCESS) {
			ret = parse_ret;
			break;
		}

		buffer_left -= block_size;
		buffer += block_size;
	}

	UVC_EXIT(ret);
	return ret;
}


uvc_error_t uvc_parse_vc_header(uvc_device_t *dev, uvc_device_info_t *info,
		const unsigned char *block, size_t block_size) {
	size_t i;
	uvc_error_t scan_ret, ret = UVC_SUCCESS;

	UVC_ENTER();

	

	info->ctrl_if.bcdUVC = SW_TO_SHORT(&block[3]);

	switch (info->ctrl_if.bcdUVC) {
	case 0x0100:
	case 0x010a:
	case 0x0110:
	case 0x0150:	
		break;
	default:
		UVC_EXIT(UVC_ERROR_NOT_SUPPORTED);
		return UVC_ERROR_NOT_SUPPORTED;
	}

	for (i = 12; i < block_size; ++i) {
		scan_ret = uvc_scan_streaming(dev, info, block[i]);
		if (scan_ret != UVC_SUCCESS) {
			ret = scan_ret;
			break;
		}
	}

	UVC_EXIT(ret);
	return ret;
}


uvc_error_t uvc_parse_vc_input_terminal(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size) {
	uvc_input_terminal_t *term;
	size_t i;

	UVC_ENTER();

	
	if (SW_TO_SHORT(&block[4]) != UVC_ITT_CAMERA) {
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS;
	}

	term = calloc(1, sizeof(*term));

	term->bTerminalID = block[3];
	term->wTerminalType = SW_TO_SHORT(&block[4]);
	term->wObjectiveFocalLengthMin = SW_TO_SHORT(&block[8]);
	term->wObjectiveFocalLengthMax = SW_TO_SHORT(&block[10]);
	term->wOcularFocalLength = SW_TO_SHORT(&block[12]);
	term->request = (term->bTerminalID << 8) | info->ctrl_if.bInterfaceNumber;
	term->bmControls = 0;	
	for (i = 14 + block[14]; i >= 15; i--)
		term->bmControls = block[i] + (term->bmControls << 8);

	DL_APPEND(info->ctrl_if.input_term_descs, term);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vc_output_terminal(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size) {
	uvc_output_terminal_t *term;
	size_t i;

	UVC_ENTER();

	
	if (SW_TO_SHORT(&block[4]) != UVC_OTT_DISPLAY) {
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS;
	}

	term = calloc(1, sizeof(*term));

	term->bTerminalID = block[3];
	term->wTerminalType = SW_TO_SHORT(&block[4]);
	term->bAssocTerminal = block[6];
	term->bSourceID = block[7];
	term->iTerminal = block[8];
	term->request = (term->bTerminalID << 8) | info->ctrl_if.bInterfaceNumber;	
	

	DL_APPEND(info->ctrl_if.output_term_descs, term);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vc_processing_unit(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size) {
	uvc_processing_unit_t *unit;
	size_t i;

	UVC_ENTER();

	unit = calloc(1, sizeof(*unit));
	unit->bUnitID = block[3];
	unit->bSourceID = block[4];
	unit->request = (unit->bUnitID << 8) | info->ctrl_if.bInterfaceNumber;	

	unit->bmControls = 0;	
	for (i = 7 + block[7]; i >= 8; i--)
		unit->bmControls = block[i] + (unit->bmControls << 8);

	DL_APPEND(info->ctrl_if.processing_unit_descs, unit);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vc_extension_unit(uvc_device_t *dev,
		uvc_device_info_t *info, const unsigned char *block, size_t block_size) {
	uvc_extension_unit_t *unit = calloc(1, sizeof(*unit));
	const uint8_t *start_of_controls;
	int size_of_controls, num_in_pins;
	int i;

	UVC_ENTER();

	unit->bUnitID = block[3];
	memcpy(unit->guidExtensionCode, &block[4], 16);

	num_in_pins = block[21];
	size_of_controls = block[22 + num_in_pins];
	start_of_controls = &block[23 + num_in_pins];
	unit->request = (unit->bUnitID << 8) | info->ctrl_if.bInterfaceNumber;

	unit->bmControls = 0;	
	for (i = size_of_controls - 1; i >= 0; i--)
		unit->bmControls = start_of_controls[i] + (unit->bmControls << 8);

	DL_APPEND(info->ctrl_if.extension_unit_descs, unit);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vc(uvc_device_t *dev, uvc_device_info_t *info,
		const unsigned char *block, size_t block_size) {
	int descriptor_subtype;
	uvc_error_t ret = UVC_SUCCESS;

	UVC_ENTER();

	if (block[1] != LIBUSB_DT_CS_INTERFACE) { 
		UVC_EXIT(UVC_SUCCESS);
		return UVC_SUCCESS; 
	}

	descriptor_subtype = block[2];

	switch (descriptor_subtype) {
	case UVC_VC_HEADER:
		ret = uvc_parse_vc_header(dev, info, block, block_size);
		break;
	case UVC_VC_INPUT_TERMINAL:
		ret = uvc_parse_vc_input_terminal(dev, info, block, block_size);
		break;
	case UVC_VC_OUTPUT_TERMINAL:
		break;
	case UVC_VC_SELECTOR_UNIT:
		break;
	case UVC_VC_PROCESSING_UNIT:
		ret = uvc_parse_vc_processing_unit(dev, info, block, block_size);
		break;
	case UVC_VC_EXTENSION_UNIT:
		ret = uvc_parse_vc_extension_unit(dev, info, block, block_size);
		break;
	default:
		LOGW("UVC_ERROR_INVALID_DEVICE:descriptor_subtype=0x%02x", descriptor_subtype);
		ret = UVC_ERROR_INVALID_DEVICE;
	}

	UVC_EXIT(ret);
	return ret;
}


uvc_error_t uvc_scan_streaming(uvc_device_t *dev, uvc_device_info_t *info,
		int interface_idx) {
	const struct libusb_interface_descriptor *if_desc;
	const unsigned char *buffer;
	size_t buffer_left, block_size;
	uvc_error_t ret, parse_ret;
	uvc_streaming_interface_t *stream_if;

	UVC_ENTER();

	ret = UVC_SUCCESS;

	if_desc = &(info->config->interface[interface_idx].altsetting[0]);
	buffer = if_desc->extra;
	buffer_left = if_desc->extra_length;
	
	if (UNLIKELY(!buffer || !buffer_left)) {
		if (if_desc->bNumEndpoints && if_desc->endpoint) {
			
			buffer = if_desc->endpoint[0].extra;
			buffer_left = if_desc->endpoint[0].extra_length;
		}
	}
	stream_if = calloc(1, sizeof(*stream_if));
	stream_if->parent = info;
	stream_if->bInterfaceNumber = if_desc->bInterfaceNumber;
	DL_APPEND(info->stream_ifs, stream_if);

	if (LIKELY(buffer_left >= 3)) {
		while (buffer_left >= 3) {
			block_size = buffer[0];

			parse_ret = uvc_parse_vs(dev, info, stream_if, buffer, block_size);

			if (parse_ret != UVC_SUCCESS) {
				ret = parse_ret;
				break;
			}

			buffer_left -= block_size;
			buffer += block_size;
		}
	} else {
		LOGW("This VideoStreaming interface has no extra data");
	}

	UVC_EXIT(ret);
	return ret;
}


uvc_error_t uvc_parse_vs_input_header(uvc_streaming_interface_t *stream_if,
		const unsigned char *block, size_t block_size) {
	UVC_ENTER();

	stream_if->bEndpointAddress = block[6] & 0x8f;
	stream_if->bTerminalLink = block[8];
	stream_if->bmInfo = block[7];	
	stream_if->bStillCaptureMethod = block[9];	
	stream_if->bTriggerSupport = block[10];	
	stream_if->bTriggerUsage = block[11];	
	stream_if->bmaControls = NULL;
	const uint8_t n = block[12];
	if (LIKELY(n)) {
		const uint8_t p = (block_size - 13) / n;
		if (LIKELY(p)) {
			uint64_t *bmaControls = (uint64_t *)calloc(p, sizeof(uint64_t));
			stream_if->bmaControls = bmaControls;
			const uint8_t *bma;
			int pp, nn;
			for (pp = 1; pp <= p; pp++) {
				bma = &block[12 + pp * n];
				for (nn = n - 1; nn >= 0; --nn) {
					*bmaControls = *bma-- + (*bmaControls << 8);
				}
				bmaControls++;
			}
		}
	}
	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vs_format_uncompressed(
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size) {
	UVC_ENTER();

	uvc_format_desc_t *format = calloc(1, sizeof(*format));

	format->parent = stream_if;
	format->bDescriptorSubtype = block[2];
	format->bFormatIndex = block[3];
	
	
	memcpy(format->guidFormat, &block[5], 16);
	format->bBitsPerPixel = block[21];
	format->bDefaultFrameIndex = block[22];
	format->bAspectRatioX = block[23];
	format->bAspectRatioY = block[24];
	format->bmInterlaceFlags = block[25];
	format->bCopyProtect = block[26];

	DL_APPEND(stream_if->format_descs, format);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vs_frame_format(uvc_streaming_interface_t *stream_if,
	const unsigned char *block, size_t block_size) {
	UVC_ENTER();

	uvc_format_desc_t *format = calloc(1, sizeof(*format));

	format->parent = stream_if;
	format->bDescriptorSubtype = block[2];
	format->bFormatIndex = block[3];
	format->bNumFrameDescriptors = block[4];
	memcpy(format->guidFormat, &block[5], 16);
	format->bBitsPerPixel = block[21];
	format->bDefaultFrameIndex = block[22];
	format->bAspectRatioX = block[23];
	format->bAspectRatioY = block[24];
	format->bmInterlaceFlags = block[25];
	format->bCopyProtect = block[26];
	format->bVariableSize = block[27];

	DL_APPEND(stream_if->format_descs, format);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vs_format_mjpeg(uvc_streaming_interface_t *stream_if,
		const unsigned char *block, size_t block_size) {
	UVC_ENTER();

	uvc_format_desc_t *format = calloc(1, sizeof(*format));

	format->parent = stream_if;
	format->bDescriptorSubtype = block[2];
	format->bFormatIndex = block[3];
	memcpy(format->fourccFormat, "MJPG", 4);
	format->bmFlags = block[5];
	format->bBitsPerPixel = 0;
	format->bDefaultFrameIndex = block[6];
	format->bAspectRatioX = block[7];
	format->bAspectRatioY = block[8];
	format->bmInterlaceFlags = block[9];
	format->bCopyProtect = block[10];

	DL_APPEND(stream_if->format_descs, format);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vs_frame_frame(uvc_streaming_interface_t *stream_if,
					    const unsigned char *block,
					    size_t block_size) {
  uvc_format_desc_t *format;
  uvc_frame_desc_t *frame;

  const unsigned char *p;
  int i;

  UVC_ENTER();

  format = stream_if->format_descs->prev;
  frame = calloc(1, sizeof(*frame));

  frame->parent = format;

  frame->bDescriptorSubtype = block[2];
  frame->bFrameIndex = block[3];
  frame->bmCapabilities = block[4];
  frame->wWidth = block[5] + (block[6] << 8);
  frame->wHeight = block[7] + (block[8] << 8);
  frame->dwMinBitRate = DW_TO_INT(&block[9]);
  frame->dwMaxBitRate = DW_TO_INT(&block[13]);
  frame->dwDefaultFrameInterval = DW_TO_INT(&block[17]);
  frame->bFrameIntervalType = block[21];
  frame->dwBytesPerLine = DW_TO_INT(&block[22]);

  if (block[21] == 0) {
    frame->dwMinFrameInterval = DW_TO_INT(&block[26]);
    frame->dwMaxFrameInterval = DW_TO_INT(&block[30]);
    frame->dwFrameIntervalStep = DW_TO_INT(&block[34]);
  } else {
    frame->intervals = calloc(block[21] + 1, sizeof(frame->intervals[0]));
    p = &block[26];

    for (i = 0; i < block[21]; ++i) {
      frame->intervals[i] = DW_TO_INT(p);
      p += 4;
    }
    frame->intervals[block[21]] = 0;
  }

  DL_APPEND(format->frame_descs, frame);

  UVC_EXIT(UVC_SUCCESS);
  return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vs_frame_uncompressed(
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size) {
	uvc_format_desc_t *format;
	uvc_frame_desc_t *frame;
	uint8_t frame_type;
	uint8_t n;
	uint32_t interval;

	const unsigned char *p;
	int i;

	UVC_ENTER();

	format = stream_if->format_descs->prev;
	frame = calloc(1, sizeof(*frame));

	frame->parent = format;

	frame_type = frame->bDescriptorSubtype = block[2];
	frame->bFrameIndex = block[3];
	frame->bmCapabilities = block[4];
	frame->wWidth = block[5] + (block[6] << 8);
	frame->wHeight = block[7] + (block[8] << 8);
	frame->dwMinBitRate = DW_TO_INT(&block[9]);
	frame->dwMaxBitRate = DW_TO_INT(&block[13]);
	frame->dwMaxVideoFrameBufferSize = DW_TO_INT(&block[17]);
	frame->dwDefaultFrameInterval = DW_TO_INT(&block[21]);
	n = frame->bFrameIntervalType = block[25];

	if (!n) {
		frame->dwMinFrameInterval = DW_TO_INT(&block[26]);
		frame->dwMaxFrameInterval = DW_TO_INT(&block[30]);
		frame->dwFrameIntervalStep = DW_TO_INT(&block[34]);
	} else {
		frame->intervals = calloc(n + 1, sizeof(frame->intervals[0]));
		p = &block[26];

		for (i = 0; i < n; ++i) {
			interval = DW_TO_INT(p);
			frame->intervals[i] = interval ? interval : 1;
			p += 4;
		}
		frame->intervals[n] = 0;
		
		frame->dwDefaultFrameInterval
			= MIN(frame->intervals[n-1],
				MAX(frame->intervals[0], frame->dwDefaultFrameInterval));
	}
	
	if (frame_type == UVC_VS_FRAME_UNCOMPRESSED) {
		frame->dwMaxVideoFrameBufferSize
			= format->bBitsPerPixel * frame->wWidth * frame->wHeight / 8;
	}
	
	
	DL_APPEND(format->frame_descs, frame);

	UVC_EXIT(UVC_SUCCESS);
	return UVC_SUCCESS;
}


uvc_error_t uvc_parse_vs(uvc_device_t *dev, uvc_device_info_t *info,
		uvc_streaming_interface_t *stream_if, const unsigned char *block,
		size_t block_size) {
	uvc_error_t ret;
	int descriptor_subtype;

	UVC_ENTER();

	ret = UVC_SUCCESS;
	descriptor_subtype = block[2];

	switch (descriptor_subtype) {
	case UVC_VS_INPUT_HEADER:
		ret = uvc_parse_vs_input_header(stream_if, block, block_size);
		break;


	case UVC_VS_FORMAT_UNCOMPRESSED:
		ret = uvc_parse_vs_format_uncompressed(stream_if, block, block_size);
		break;
	case UVC_VS_FORMAT_MJPEG:
		ret = uvc_parse_vs_format_mjpeg(stream_if, block, block_size);
		break;
	case UVC_VS_FRAME_UNCOMPRESSED:
	case UVC_VS_FRAME_MJPEG:
		ret = uvc_parse_vs_frame_uncompressed(stream_if, block, block_size);
		break;
	case UVC_VS_FORMAT_FRAME_BASED:
		ret = uvc_parse_vs_frame_format(stream_if, block, block_size );
		break;
	case UVC_VS_FRAME_FRAME_BASED:
		ret = uvc_parse_vs_frame_frame(stream_if, block, block_size );
		break;


	default:
		
		LOGV("unsupported descriptor_subtype(0x%02x)", descriptor_subtype);
		break;
	}

	UVC_EXIT(ret);
	return ret;
}


void uvc_free_devh(uvc_device_handle_t *devh) {
	UVC_ENTER();

	pthread_mutex_destroy(&devh->status_mutex);	
	if (devh->info)
		uvc_free_device_info(devh->info);

	if (devh->status_xfer)
		libusb_free_transfer(devh->status_xfer);

	free(devh);

	UVC_EXIT_VOID();
}


void uvc_close(uvc_device_handle_t *devh) {

	UVC_ENTER();

	uvc_context_t *ctx = devh->dev->ctx;

	if (devh->streams)
		uvc_stop_streaming(devh);

	uvc_release_if(devh, devh->info->ctrl_if.bInterfaceNumber);

#if !UVC_DETACH_ATTACH
	
	libusb_set_auto_detach_kernel_driver(devh->usb_devh, 0);
#endif
	
	if (ctx->own_usb_ctx && ctx->open_devices == devh && devh->next == NULL) {
		ctx->kill_handler_thread = 1;
		libusb_close(devh->usb_devh);
		pthread_join(ctx->handler_thread, NULL);
	} else {
		libusb_close(devh->usb_devh);
	}

	DL_DELETE(ctx->open_devices, devh);

	uvc_unref_device(devh->dev);

	uvc_free_devh(devh);

	UVC_EXIT_VOID();
}

uvc_error_t uvc_set_reset_altsetting(uvc_device_handle_t *devh, uint8_t reset_on_release_if) {
	if (UNLIKELY(!devh))
		RETURN(UVC_ERROR_INVALID_PARAM, uvc_error_t);
	devh->reset_on_release_if = reset_on_release_if;
	RETURN(UVC_SUCCESS, uvc_error_t);
}


size_t uvc_num_devices(uvc_context_t *ctx) {
	size_t count = 0;

	uvc_device_handle_t *devh;

	UVC_ENTER();

	DL_FOREACH(ctx->open_devices, devh)
	{
		count++;
	}

	UVC_EXIT((int) count);
	return count;
}

void uvc_process_control_status(uvc_device_handle_t *devh, unsigned char *data, int len) {
	enum uvc_status_class status_class;
	uint8_t originator = 0, selector = 0, event = 0;
	enum uvc_status_attribute attribute = UVC_STATUS_ATTRIBUTE_UNKNOWN;
	void *content = NULL;
	size_t content_len = 0;
	int found_entity = 0;
	struct uvc_input_terminal *input_terminal;
	struct uvc_processing_unit *processing_unit;

	UVC_ENTER();

	if (len < 5) {
		UVC_DEBUG("Short read of VideoControl status update (%d bytes)", len);
		UVC_EXIT_VOID();
		return;
	}

	originator = data[1];
	event = data[2];
	selector = data[3];

	if (originator == 0) {
		UVC_DEBUG("Unhandled update from VC interface");
		UVC_EXIT_VOID();
		return;  
	}

	if (event != 0) {
		UVC_DEBUG("Unhandled VC event %d", (int) event);
		UVC_EXIT_VOID();
		return;
	}

	

	DL_FOREACH(devh->info->ctrl_if.input_term_descs, input_terminal) {
		if (input_terminal->bTerminalID == originator) {
			status_class = UVC_STATUS_CLASS_CONTROL_CAMERA;
			found_entity = 1;
			break;
		}
	}

	if (!found_entity) {
		DL_FOREACH(devh->info->ctrl_if.processing_unit_descs, processing_unit) {
			if (processing_unit->bUnitID == originator) {
				status_class = UVC_STATUS_CLASS_CONTROL_PROCESSING;
				found_entity = 1;
				break;
			}
		}
	}

	if (!found_entity) {
		UVC_DEBUG("Got status update for unknown VideoControl entity %d",
				  (int) originator);
		UVC_EXIT_VOID();
		return;
	}

	attribute = data[4];
	content = data + 5;
	content_len = len - 5;

	UVC_DEBUG("Event: class=%d, event=%d, selector=%d, attribute=%d, content_len=%zd",
			  status_class, event, selector, attribute, content_len);

	pthread_mutex_lock(&devh->status_mutex);	
	{
	if(devh->status_cb) {
		UVC_DEBUG("Running user-supplied status callback");
		devh->status_cb(status_class,
						event,
						selector,
						attribute,
						content, content_len,
						devh->status_user_ptr);
	}
	}
	pthread_mutex_unlock(&devh->status_mutex);	

	UVC_EXIT_VOID();
}

void uvc_process_streaming_status(uvc_device_handle_t *devh, unsigned char *data, int len) {

	UVC_ENTER();

	if (len < 3) {
		UVC_DEBUG("Invalid streaming status event received.\n");
		UVC_EXIT_VOID();
		return;
	}

	if (data[2] == 0) {
		if (len < 4) {
			UVC_DEBUG("Short read of status update (%d bytes)", len);
			UVC_EXIT_VOID();
			return;
		}
		UVC_DEBUG("Button (intf %u) %s len %d\n", data[1], data[3] ? "pressed" : "released", len);

		pthread_mutex_lock(&devh->status_mutex);	
		{
			if(devh->button_cb) {
				UVC_DEBUG("Running user-supplied button callback");
				devh->button_cb(data[1],
								data[3],
								devh->button_user_ptr);
		}
		}
		pthread_mutex_unlock(&devh->status_mutex);	
	} else {
		UVC_DEBUG("Stream %u error event %02x %02x len %d.\n", data[1], data[2], data[3], len);
	}

	UVC_EXIT_VOID();
}

void uvc_process_status_xfer(uvc_device_handle_t *devh, struct libusb_transfer *transfer) {

	UVC_ENTER();

	

	if (transfer->actual_length > 0) {
		switch (transfer->buffer[0] & 0x0f) {
			case 1: 
				uvc_process_control_status(devh, transfer->buffer, transfer->actual_length);
				break;
			case 2:  
				uvc_process_streaming_status(devh, transfer->buffer, transfer->actual_length);
				break;
		}
	}

	UVC_EXIT_VOID();
}


void _uvc_status_callback(struct libusb_transfer *transfer) {
	UVC_ENTER();

	uvc_device_handle_t *devh = (uvc_device_handle_t *) transfer->user_data;

	switch (transfer->status) {
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_NO_DEVICE:
		UVC_DEBUG("not processing/resubmitting, status = %d", transfer->status);
		UVC_EXIT_VOID();
		return;
	case LIBUSB_TRANSFER_COMPLETED:
		uvc_process_status_xfer(devh, transfer);
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_OVERFLOW:
		UVC_DEBUG("retrying transfer, status = %d", transfer->status);
		break;
	}

	uvc_error_t ret = libusb_submit_transfer(transfer);
	UVC_DEBUG("libusb_submit_transfer() = %d", ret);

	UVC_EXIT_VOID();
}


void uvc_set_status_callback(uvc_device_handle_t *devh,
		uvc_status_callback_t cb, void *user_ptr) {
	UVC_ENTER();

	pthread_mutex_lock(&devh->status_mutex);	
	{
		devh->status_cb = cb;
		devh->status_user_ptr = user_ptr;
	}
	pthread_mutex_unlock(&devh->status_mutex);	

	UVC_EXIT_VOID();
}


void uvc_set_button_callback(uvc_device_handle_t *devh,
		uvc_button_callback_t cb, void *user_ptr) {
	UVC_ENTER();

	pthread_mutex_lock(&devh->status_mutex);	
	{
		devh->button_cb = cb;
		devh->button_user_ptr = user_ptr;
	}
	pthread_mutex_unlock(&devh->status_mutex);	

	UVC_EXIT_VOID();
}


const uvc_format_desc_t *uvc_get_format_descs(uvc_device_handle_t *devh) {
  return devh->info->stream_ifs->format_descs;
}
