



#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "libusbi.h"












static inline int is_known_descriptor_type(int type) {
	return ((type == LIBUSB_DT_ENDPOINT)
		|| (type == LIBUSB_DT_INTERFACE)
		|| (type == LIBUSB_DT_CONFIG)
		|| (type == LIBUSB_DT_DEVICE)
		|| (type == LIBUSB_DT_ASSOCIATION) );
}


int usbi_parse_descriptor(const unsigned char *source, const char *descriptor,
	void *dest, int host_endian)
{
	const unsigned char *sp = source;
	unsigned char *dp = dest;
	uint16_t w;
	const char *cp;
	uint32_t d;

	for (cp = descriptor; *cp; cp++) {
		switch (*cp) {
			case 'b':	
				*dp++ = *sp++;
				break;
			case 'w':	
				dp += ((uintptr_t)dp & 1);	

				if (host_endian) {
					memcpy(dp, sp, 2);
				} else {
					w = (sp[1] << 8) | sp[0];
					*((uint16_t *)dp) = w;
				}
				sp += 2;
				dp += 2;
				break;
			case 'd':	
				dp += ((uintptr_t)dp & 1);	

				if (host_endian) {
					memcpy(dp, sp, 4);
				} else {
					d = (sp[3] << 24) | (sp[2] << 16) |
						(sp[1] << 8) | sp[0];
					*((uint32_t *)dp) = d;
				}
				sp += 4;
				dp += 4;
				break;
			case 'u':	
				memcpy(dp, sp, 16);
				sp += 16;
				dp += 16;
				break;
		}
	}

	return (int) (sp - source);
}

static void clear_endpoint(struct libusb_endpoint_descriptor *endpoint)
{
	if LIKELY(endpoint && endpoint->extra) {
		free((unsigned char *) endpoint->extra);
		endpoint->extra = NULL; 
		endpoint->extra_length = 0;
	}
}

static int parse_endpoint(struct libusb_context *ctx,
	struct libusb_endpoint_descriptor *endpoint, unsigned char *buffer,
	int size, int host_endian)
{
	ENTER();

	struct usb_descriptor_header header;
	unsigned char *extra;
	unsigned char *begin;
	int parsed = 0;
	int len;

	if UNLIKELY(size < LIBUSB_DT_HEADER_SIZE) {
		usbi_err(ctx, "short endpoint descriptor read %d/%d",
			 size, LIBUSB_DT_HEADER_SIZE);
		RETURN(LIBUSB_ERROR_IO, int);
	}

	usbi_parse_descriptor(buffer, "bb", &header, 0);
	if UNLIKELY(header.bDescriptorType != LIBUSB_DT_ENDPOINT) {
		usbi_err(ctx, "unexpected descriptor %x (expected %x)",
			header.bDescriptorType, LIBUSB_DT_ENDPOINT);
		RETURN(parsed, int);
	}
	if UNLIKELY(header.bLength > size) {
		usbi_warn(ctx, "short endpoint descriptor read %d/%d",
			  size, header.bLength);
		RETURN(parsed, int);
	}
	if (header.bLength >= LIBUSB_DT_ENDPOINT_AUDIO_SIZE)
		usbi_parse_descriptor(buffer, "bbbbwbbb", endpoint, host_endian);
	else if (header.bLength >= LIBUSB_DT_ENDPOINT_SIZE)
		usbi_parse_descriptor(buffer, "bbbbwb", endpoint, host_endian);
	else {
		usbi_err(ctx, "invalid endpoint bLength (%d)", header.bLength);
		RETURN(LIBUSB_ERROR_IO, int);
	}

	buffer += header.bLength;
	size -= header.bLength;
	parsed += header.bLength;

	
	
	begin = buffer;
	while (size >= LIBUSB_DT_HEADER_SIZE) {
		usbi_parse_descriptor(buffer, "bb", &header, 0);
		if UNLIKELY(header.bLength < LIBUSB_DT_HEADER_SIZE) {
			usbi_err(ctx, "invalid extra ep desc len (%d)",
				 header.bLength);
			RETURN(LIBUSB_ERROR_IO, int);
		} else if (header.bLength > size) {
			usbi_warn(ctx, "short extra ep desc read %d/%d",
				  size, header.bLength);
			RETURN(parsed, int);
		}

		
		if (is_known_descriptor_type(header.bDescriptorType))
			break;

		usbi_dbg("skipping descriptor 0x%02x", header.bDescriptorType);
		buffer += header.bLength;
		size -= header.bLength;
		parsed += header.bLength;
	}

	
	
	len = (int)(buffer - begin);
	if (!len) {
		endpoint->extra = NULL;
		endpoint->extra_length = 0;
		RETURN(parsed, int);
	}

	endpoint->extra = extra = malloc(len);
	if UNLIKELY(!extra) {
		endpoint->extra_length = 0;
		RETURN(LIBUSB_ERROR_NO_MEM, int);
	}

	memcpy(extra, begin, len);
	endpoint->extra_length = len;

	RETURN(parsed, int);
}

static void clear_interface(struct libusb_interface *usb_interface)
{
	int i;
	int j;

	if (usb_interface->altsetting) {
		for (i = 0; i < usb_interface->num_altsetting; i++) {
			struct libusb_interface_descriptor *ifp =
				(struct libusb_interface_descriptor *)
				usb_interface->altsetting + i;
			if (ifp->extra)
				free((void *) ifp->extra);
			if (ifp->endpoint) {
				for (j = 0; j < ifp->bNumEndpoints; j++)
					clear_endpoint((struct libusb_endpoint_descriptor *)
						ifp->endpoint + j);
				free((void *) ifp->endpoint);
			}
		}
		free((void *) usb_interface->altsetting);
		usb_interface->altsetting = NULL;
	}

}

static int parse_interface(libusb_context *ctx,
	struct libusb_interface *usb_interface, unsigned char *buffer, int size,
	int host_endian)
{
	ENTER();

	int i;
	int len;
	int r;
	int parsed = 0;
	int interface_number = -1;
	size_t tmp;
	struct usb_descriptor_header header;
	struct libusb_interface_descriptor *ifp;
	unsigned char *begin;

	usb_interface->num_altsetting = 0;

	while (size >= LIBUSB_DT_INTERFACE_SIZE) {
		struct libusb_interface_descriptor *altsetting =
			(struct libusb_interface_descriptor *) usb_interface->altsetting;
		altsetting = usbi_reallocf(altsetting,
			sizeof(struct libusb_interface_descriptor) * (usb_interface->num_altsetting + 1));
		if UNLIKELY(!altsetting) {
			r = LIBUSB_ERROR_NO_MEM;
			goto err;
		}
		usb_interface->altsetting = altsetting;

		ifp = altsetting + usb_interface->num_altsetting;
		usbi_parse_descriptor(buffer, "bbbbbbbbb", ifp, 0);
		if UNLIKELY(ifp->bDescriptorType != LIBUSB_DT_INTERFACE) {
			usbi_err(ctx, "unexpected descriptor %x (expected %x)",
				 ifp->bDescriptorType, LIBUSB_DT_INTERFACE);
			RETURN(parsed, int);
		}
		if UNLIKELY(ifp->bLength < LIBUSB_DT_INTERFACE_SIZE) {
			usbi_err(ctx, "invalid interface bLength (%d)",
				 ifp->bLength);
			r = LIBUSB_ERROR_IO;
			goto err;
		}
		if UNLIKELY(ifp->bLength > size) {
			usbi_warn(ctx, "short intf descriptor read %d/%d",
				 size, ifp->bLength);
			RETURN(parsed, int);
		}
		if UNLIKELY(ifp->bNumEndpoints > USB_MAXENDPOINTS) {
			usbi_err(ctx, "too many endpoints (%d)", ifp->bNumEndpoints);
			r = LIBUSB_ERROR_IO;
			goto err;
		}

		usb_interface->num_altsetting++;
		ifp->extra = NULL;
		ifp->extra_length = 0;
		ifp->endpoint = NULL;

		if (interface_number == -1)
			interface_number = ifp->bInterfaceNumber;

		
		buffer += ifp->bLength;
		parsed += ifp->bLength;
		size -= ifp->bLength;

		begin = buffer;

		
		while (size >= LIBUSB_DT_HEADER_SIZE) {
			usbi_parse_descriptor(buffer, "bb", &header, 0);
			if UNLIKELY(header.bLength < LIBUSB_DT_HEADER_SIZE) {
				usbi_err(ctx,
					 "invalid extra intf desc len (%d)",
					 header.bLength);
				r = LIBUSB_ERROR_IO;
				goto err;
			} else if (header.bLength > size) {
				usbi_warn(ctx,
					  "short extra intf desc read %d/%d",
					  size, header.bLength);
				RETURN(parsed, int);
			}

			MARK("bDescriptorType=0x%02x", header.bDescriptorType);
			
			if (is_known_descriptor_type(header.bDescriptorType))
				break;

			buffer += header.bLength;
			parsed += header.bLength;
			size -= header.bLength;
		}

		
		
		len = (int)(buffer - begin);
		if (len) {
			MARK("save unknown descriptors into ifp->extra:lebgth=%d", len);
			ifp->extra = usbi_reallocf((unsigned char *)ifp->extra, ifp->extra_length + len);
			if UNLIKELY(!ifp->extra) {
				r = LIBUSB_ERROR_NO_MEM;
				goto err;
			}
			memcpy((unsigned char *)(ifp->extra + ifp->extra_length), begin, len);
			ifp->extra_length += len;
		}

		MARK("bNumEndpoints=%d", ifp->bNumEndpoints);
		if (ifp->bNumEndpoints > 0) {
			struct libusb_endpoint_descriptor *endpoint;
			tmp = ifp->bNumEndpoints * sizeof(struct libusb_endpoint_descriptor);
			ifp->endpoint = endpoint = malloc(tmp);
			if UNLIKELY(!endpoint) {
				r = LIBUSB_ERROR_NO_MEM;
				goto err;
			}

			memset(endpoint, 0, tmp);
			for (i = 0; i < ifp->bNumEndpoints; i++) {
				MARK("parse endpoint%d", i);
				r = parse_endpoint(ctx, endpoint + i, buffer, size,
					host_endian);
				if UNLIKELY(r < 0)
					goto err;
				if (r == 0) {
					ifp->bNumEndpoints = (uint8_t)i;
					break;;
				}

				buffer += r;
				parsed += r;
				size -= r;
			}
		}

		
		ifp = (struct libusb_interface_descriptor *) buffer;
		if (size < LIBUSB_DT_INTERFACE_SIZE ||
				ifp->bDescriptorType != LIBUSB_DT_INTERFACE ||
				ifp->bInterfaceNumber != interface_number)
			RETURN(parsed, int);
	}

	RETURN(parsed, int);
err:
	clear_interface(usb_interface);
	RETURN(r, int);
}

static void clear_association(struct libusb_association_descriptor *association) {
	if LIKELY(association && association->extra) {
		free((unsigned char *) association->extra);
		association->extra = NULL;
		association->extra_length = 0;
	}
}

static int parse_association(struct libusb_context *ctx,
		struct libusb_config_descriptor *config, unsigned char *buffer,
	int size, int host_endian) {

	ENTER();

	struct usb_descriptor_header header;
	struct libusb_association_descriptor *association, *temp;
	unsigned char *begin;
	int parsed = 0;
	int len;

	if UNLIKELY(size < LIBUSB_DT_HEADER_SIZE) {
		usbi_err(ctx, "short association descriptor read %d/%d",
			 size, LIBUSB_DT_HEADER_SIZE);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	
	usbi_parse_descriptor(buffer, "bb", &header, 0);
	if UNLIKELY(header.bDescriptorType != LIBUSB_DT_ASSOCIATION) {	
		usbi_err(ctx, "unexpected descriptor %x (expected %x)",
			header.bDescriptorType, LIBUSB_DT_ASSOCIATION);
		RETURN(parsed, int);	
	}
	if UNLIKELY(header.bLength > size) {	
		usbi_warn(ctx, "short association descriptor read %d/%d",
			  size, header.bLength);
		RETURN(parsed, int);	
	}
	if (header.bLength >= LIBUSB_DT_ASSOCIATION_SIZE) {
		config->association_descriptor = usbi_reallocf(config->association_descriptor,
			sizeof(struct libusb_association_descriptor) * (config->num_associations + 1));
		if UNLIKELY(!config->association_descriptor) {
			parsed = LIBUSB_ERROR_NO_MEM;
			goto err;
		}
		association = config->association_descriptor + config->num_associations;
		association->extra = NULL;
		association->extra_length = 0;
		len = usbi_parse_descriptor(buffer, "bbbbbbbb", association, host_endian);
		if LIKELY(len > 0) {
			config->num_associations++;
#if 0
			LOGI("\t association:bLength=%d", association->bLength);
			LOGI("\t association:bDescriptorType=0x%02d", association->bDescriptorType);
			LOGI("\t association:bFirstInterface=%d", association->bFirstInterface);
			LOGI("\t association:bInterfaceCount=%d", association->bInterfaceCount);
			LOGI("\t association:bFunctionClass=0x%02x", association->bFunctionClass);
			LOGI("\t association:bFunctionSubClass=0x%02x", association->bFunctionSubClass);
			LOGI("\t association:bFunctionProtocol=0x%02x", association->bFunctionProtocol);
			LOGI("\t association:iFunction=%d", association->iFunction);
#endif
		} else {
			
			config->association_descriptor = usbi_reallocf(association,
				sizeof(struct libusb_association_descriptor) * config->num_associations);
		}
	} else {
		
		usbi_err(ctx, "invalid interface association descriptor bLength (%d)", header.bLength);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	
	buffer += header.bLength;
	size -= header.bLength;
	parsed += header.bLength;

	
	begin = buffer;
	while (size >= LIBUSB_DT_HEADER_SIZE) {
		usbi_parse_descriptor(buffer, "bb", &header, 0);
		if UNLIKELY(header.bLength < LIBUSB_DT_HEADER_SIZE) {
			usbi_err(ctx, "invalid extra ia desc len (%d)",
				 header.bLength);
			RETURN(LIBUSB_ERROR_IO, int);
		} else if (header.bLength > size) {
			usbi_warn(ctx, "short extra ia desc read %d/%d",
				  size, header.bLength);
			RETURN(parsed, int);
		}

		MARK("bDescriptorType=0x%02x", header.bDescriptorType);
		
		if (is_known_descriptor_type(header.bDescriptorType))
			break;

		usbi_dbg("skipping descriptor 0x%02x", header.bDescriptorType);
		buffer += header.bLength;
		size -= header.bLength;
		parsed += header.bLength;
	}

	
	len = (int)(buffer - begin);
	if (!len) {
		RETURN(parsed, int);
	}

	MARK("save unknown descriptors into config->extra:length=%d", len);
	config->extra = usbi_reallocf((unsigned char *)config->extra, config->extra_length + len);
	if UNLIKELY(!config->extra) {
		config->extra_length = 0;
		RETURN(LIBUSB_ERROR_NO_MEM, int);
	}
	memcpy((unsigned char *)config->extra + config->extra_length, begin, len);
	config->extra_length += len;

	RETURN(parsed, int);
err:
	clear_association(config->association_descriptor);
	config->association_descriptor = NULL;
	RETURN(parsed, int);
}

static void clear_configuration(struct libusb_config_descriptor *config)
{
	if UNLIKELY(!config) return;

	if LIKELY(config->interface) {
		int i;
		for (i = 0; i < config->bNumInterfaces; i++)
			clear_interface((struct libusb_interface *)
				config->interface + i);
		free((void *) config->interface);
		config->interface = NULL;	
	}
	if (config->extra) {
		free((void *) config->extra);
		config->extra = NULL;	
	}
	if LIKELY(config->association_descriptor) {
		int i;
		for (i = 0; i < config->num_associations; i++)
			clear_association(config->association_descriptor + i);
		free((void *)config->association_descriptor);
		config->association_descriptor = NULL;
	}
}

static int parse_configuration(struct libusb_context *ctx,
	struct libusb_config_descriptor *config, unsigned char *buffer,
	int size, int host_endian) {

	ENTER();

	int parsed_if;
	int r;
	size_t tmp;
	struct usb_descriptor_header header;
	struct libusb_interface *usb_interface;
	struct libusb_association_descriptor *association_desc;

	if UNLIKELY(size < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(ctx, "short config descriptor read %d/%d",
			 size, LIBUSB_DT_CONFIG_SIZE);
		RETURN(LIBUSB_ERROR_IO, int);
	}

	usbi_parse_descriptor(buffer, "bbwbbbbb", config, host_endian);
	if UNLIKELY(config->bDescriptorType != LIBUSB_DT_CONFIG) {
		usbi_err(ctx, "unexpected descriptor %x (expected %x)",
			 config->bDescriptorType, LIBUSB_DT_CONFIG);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	if UNLIKELY(config->bLength < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(ctx, "invalid config bLength (%d)", config->bLength);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	if UNLIKELY(config->bLength > size) {
		usbi_err(ctx, "short config descriptor read %d/%d",
			 size, config->bLength);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	if UNLIKELY(config->bNumInterfaces > USB_MAXINTERFACES) {
		usbi_err(ctx, "too many interfaces (%d)", config->bNumInterfaces);
		RETURN(LIBUSB_ERROR_IO, int);
	}
	
	tmp = config->bNumInterfaces * sizeof(struct libusb_interface);
	config->interface = usb_interface = malloc(tmp);
	
	if UNLIKELY(!config->interface)
		RETURN(LIBUSB_ERROR_NO_MEM, int);

	config->association_descriptor = NULL;
	config->num_associations = 0;

	memset(usb_interface, 0, tmp);
	buffer += config->bLength;
	size -= config->bLength;

	config->extra = NULL;
	config->extra_length = 0;
	MARK("bNumInterfaces=%d", config->bNumInterfaces);
	for (parsed_if = 0; (parsed_if < config->bNumInterfaces) && (size > 0); ) {
		int len;
		unsigned char *begin;

		
		begin = buffer;
		while (size >= LIBUSB_DT_HEADER_SIZE) {
			usbi_parse_descriptor(buffer, "bb", &header, 0);

			if UNLIKELY(header.bLength < LIBUSB_DT_HEADER_SIZE) {
				usbi_err(ctx,
					 "invalid extra config desc len (%d)",
					 header.bLength);
				r = LIBUSB_ERROR_IO;
				goto err;
			} else if UNLIKELY(header.bLength > size) {
				usbi_warn(ctx,
					  "short extra config desc read %d/%d",
					  size, header.bLength);
				config->bNumInterfaces = (uint8_t)parsed_if;
				return size;
			}

			MARK("bDescriptorType=0x%02x", header.bDescriptorType);
			
			if (is_known_descriptor_type(header.bDescriptorType))
				break;

			usbi_dbg("skipping descriptor 0x%02x\n", header.bDescriptorType);
			buffer += header.bLength;
			size -= header.bLength;
		}

		
		
		len = (int)(buffer - begin);
		if (len) {
			MARK("save skipped unknown descriptors into config->extra:len=%d", len);
			config->extra = usbi_reallocf((void *) config->extra, config->extra_length + len);
			if UNLIKELY(!config->extra) {
				r = LIBUSB_ERROR_NO_MEM;
				goto err;
			}
			memcpy((unsigned char *)(config->extra + config->extra_length), begin, len);
			config->extra_length += len;
		}
		switch (header.bDescriptorType) {
		case LIBUSB_DT_ASSOCIATION:
			r = parse_association(ctx, config, buffer, size, host_endian);
			if (r < 0)
				goto err;
			break;
		default:
		case LIBUSB_DT_INTERFACE:
			r = parse_interface(ctx, usb_interface + parsed_if, buffer, size, host_endian);
			parsed_if++;
			if (r < 0)
				goto err;
			break;
		}
		if (r == 0) {
			config->bNumInterfaces = (uint8_t)parsed_if;
			break;
		}

		buffer += r;
		size -= r;
	}
	RETURN(size, int);

err:
	clear_configuration(config);
	RETURN(r, int);
}

#if PRINT_DIAG
static void dump_descriptors(unsigned char *buffer, int size) {
	struct usb_descriptor_header header;
	struct libusb_config_descriptor config;
	struct libusb_interface_descriptor interface;
	struct libusb_endpoint_descriptor endpoint;
	int i;

	LOGI("DUMP DESCRIPTIONS");
	for (i = 0; size >= 0; i += header.bLength, size -= header.bLength) {
		if (size == 0) {
			LOGI("END");
			return;
		}

		if (size < LIBUSB_DT_HEADER_SIZE) {
			LOGE("short descriptor read %d/2", size);
			return;
		}
		usbi_parse_descriptor(buffer + i, "bb", &header, 0);
		switch (header.bDescriptorType) {
		case LIBUSB_DT_DEVICE:
			LOGI("LIBUSB_DT_DEVICE(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_CONFIG:
			usbi_parse_descriptor(buffer, "bbwbbbbb", &config, 0);
			LOGI("LIBUSB_DT_CONFIG(0x%02x)", config.bDescriptorType);
			LOGI("\tbLength=%d", config.bLength);
			LOGI("\tbDescriptorType=0x%02x", config.bDescriptorType);
			LOGI("\twTotalLength=%d", config.wTotalLength);
			LOGI("\tbNumInterfaces=%d", config.bNumInterfaces);
			LOGI("\tbConfigurationValue=%d", config.bConfigurationValue);
			LOGI("\tiConfiguration=%d", config.iConfiguration);
			LOGI("\tbmAttributes=%d", config.bmAttributes);
			LOGI("\tMaxPower=%d", config.MaxPower);
			LOGI("\textra_length=%d", config.bLength - LIBUSB_DT_CONFIG_SIZE);
			break;
		case LIBUSB_DT_STRING:
			LOGI("LIBUSB_DT_STRING(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_INTERFACE:
			usbi_parse_descriptor(buffer + i, "bbbbbbbbb", &interface, 0);
			LOGI("LIBUSB_DT_INTERFACE(0x%02x):", header.bDescriptorType);
			LOGI("\tbLength=%d", interface.bLength);
			LOGI("\tbDescriptorType=0x%02x", interface.bDescriptorType);
			LOGI("\tbInterfaceNumber=%d", interface.bInterfaceNumber);
			LOGI("\tbAlternateSetting=%d", interface.bAlternateSetting);
			LOGI("\tbNumEndpoints=%d", interface.bNumEndpoints);
			LOGI("\tbInterfaceClass=0x%02x", interface.bInterfaceClass);
			LOGI("\tbInterfaceSubClass=0x%02x", interface.bInterfaceSubClass);
			LOGI("\tbInterfaceProtocol=0x%02x", interface.bInterfaceProtocol);
			LOGI("\tiInterface=%d", interface.iInterface);
			LOGI("\textra_length=%d", interface.bLength - LIBUSB_DT_INTERFACE_SIZE);
			break;
		case LIBUSB_DT_ENDPOINT:
			usbi_parse_descriptor(buffer + i, "bbbbwbbb", &endpoint, 0);
			LOGI("LIBUSB_DT_ENDPOINT(0x%02x):", header.bDescriptorType);
			LOGI("\tbLength=%d", endpoint.bLength);
			LOGI("\tbDescriptorType=0x%02x", endpoint.bDescriptorType);
			LOGI("\tbEndpointAddress=%d", endpoint.bEndpointAddress);
			LOGI("\tbmAttributes=%d", endpoint.bmAttributes);
			LOGI("\twMaxPacketSize=%d", endpoint.wMaxPacketSize);
			LOGI("\tbInterval=%d", endpoint.bInterval);
			LOGI("\tbRefresh=%d", endpoint.bRefresh);
			LOGI("\tbSynchAddress=%d", endpoint.bSynchAddress);
			LOGI("\textra_length=%d", endpoint.bLength - LIBUSB_DT_ENDPOINT_SIZE);
			break;
		case LIBUSB_DT_DEVICE_QUALIFIER:
			LOGI("LIBUSB_DT_DEVICE_QUALIFIER(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_QUALIFER_SIZE);
			break;
		case LIBUSB_DT_OTHER_SPEED_CONFIGURATION:
			LOGI("LIBUSB_DT_OTHER_SPEED_CONFIGURATION(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_OTHER_SPEED_SIZE);
			break;
		case LIBUSB_DT_INTERFACE_POWER:
			LOGI("LIBUSB_DT_INTERFACE_POWER(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_OTG:
			LOGI("LIBUSB_DT_OTG(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_DEBUG:
			LOGI("LIBUSB_DT_DEBUG(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_ASSOCIATION:
			LOGI("LIBUSB_DT_ASSOCIATION(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_ASSOCIATION_SIZE);
			break;
		case LIBUSB_DT_BOS:
			LOGI("LIBUSB_DT_BOS(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_BOS_SIZE);
			break;
		case LIBUSB_DT_DEVICE_CAPABILITY:
			LOGI("LIBUSB_DT_DEVICE_CAPABILITY(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			LOGI("\textra_length=%d", header.bLength - LIBUSB_DT_DEVICE_CAPABILITY_SIZE);
			break;
		case LIBUSB_DT_HID:
			LOGI("LIBUSB_DT_HID(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_HID_REPORT:
			LOGI("LIBUSB_DT_REPORT(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_HID_PHYSICAL:
			LOGI("LIBUSB_DT_PHYSICAL(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_CS_INTERFACE:
			LOGI("LIBUSB_DT_CS_INTERFACE(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_CS_ENDPOINT:
			LOGI("LIBUSB_DT_CS_ENDPOINT(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_HUB:
			LOGI("LIBUSB_DT_HUB(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_SUPERSPEED_HUB:
			LOGI("LIBUSB_DT_SUPERSPEED_HUB(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		case LIBUSB_DT_SS_ENDPOINT_COMPANION:
			LOGI("LIBUSB_DT_SS_ENDPOINT_COMPANION(0x%02x),length=%d", header.bDescriptorType, header.bLength);
			break;
		default:
			LOGI("unknown Descriptor(0x%02x),length=0x%02x", header.bDescriptorType, header.bLength);
			break;
		}
	}
}
#endif

static int raw_desc_to_config(struct libusb_context *ctx,
	unsigned char *buf, int size, int host_endian,
	struct libusb_config_descriptor **config)
{
	ENTER();

	struct libusb_config_descriptor *_config = malloc(sizeof(*_config));
	int r;
	
	if UNLIKELY(!_config)
		RETURN(LIBUSB_ERROR_NO_MEM, int);

#if PRINT_DIAG
	dump_descriptors(buf, size);
#endif
	r = parse_configuration(ctx, _config, buf, size, host_endian);
	if UNLIKELY(r < 0) {
		usbi_err(ctx, "parse_configuration failed with error %d", r);
		free(_config);
		return r;
	} else if (r > 0) {
		usbi_warn(ctx, "still %d bytes of descriptor data left", r);
	}
	
	*config = _config;
	RETURN(LIBUSB_SUCCESS, int);
}

int usbi_device_cache_descriptor(libusb_device *dev)
{
	int r, host_endian = 0;

	r = usbi_backend->get_device_descriptor(dev, (unsigned char *) &dev->device_descriptor,
						&host_endian);
	if UNLIKELY(r < 0)
		return r;

	if (!host_endian) {
		dev->device_descriptor.bcdUSB = libusb_le16_to_cpu(dev->device_descriptor.bcdUSB);
		dev->device_descriptor.idVendor = libusb_le16_to_cpu(dev->device_descriptor.idVendor);
		dev->device_descriptor.idProduct = libusb_le16_to_cpu(dev->device_descriptor.idProduct);
		dev->device_descriptor.bcdDevice = libusb_le16_to_cpu(dev->device_descriptor.bcdDevice);
	}

	return LIBUSB_SUCCESS;
}

int API_EXPORTED libusb_get_raw_descriptor(libusb_device *dev,
		unsigned char **buffer, int *descriptors_len, int *host_endian)
{
	if UNLIKELY(!buffer || !descriptors_len || !host_endian)
		return LIBUSB_ERROR_INVALID_PARAM;

	int len, r;
	r = usbi_backend->get_raw_descriptor(dev, NULL, &len, host_endian);
	if (!r) {
		unsigned char *temp = realloc(*buffer, len);
		if UNLIKELY(!temp)
			return LIBUSB_ERROR_NO_MEM;
		*buffer = temp;
		*descriptors_len = len;
		r = usbi_backend->get_raw_descriptor(dev, temp, &len, host_endian);
	}
	return r;
}


int API_EXPORTED libusb_get_device_descriptor(libusb_device *dev,
	struct libusb_device_descriptor *desc)
{
	usbi_dbg("");
	
	LOGD("desc=%p,dev=%p,device_descriptor=%p", desc, dev, &dev->device_descriptor);
	memcpy((unsigned char *) desc, (unsigned char *) &dev->device_descriptor,
	       sizeof (dev->device_descriptor));
	return 0;
}


int API_EXPORTED libusb_get_active_config_descriptor(libusb_device *dev,
	struct libusb_config_descriptor **config)
{
	struct libusb_config_descriptor _config;
	unsigned char tmp[LIBUSB_DT_CONFIG_SIZE];
	unsigned char *buf = NULL;
	int host_endian = 0;
	int r;

	r = usbi_backend->get_active_config_descriptor(dev, tmp,	
		LIBUSB_DT_CONFIG_SIZE, &host_endian);
	if UNLIKELY(r < 0)
		return r;
	if UNLIKELY(r < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(dev->ctx, "short config descriptor read %d/%d",
			 r, LIBUSB_DT_CONFIG_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(tmp, "bbw", &_config, host_endian);
	buf = malloc(_config.wTotalLength);
	if UNLIKELY(!buf)
		return LIBUSB_ERROR_NO_MEM;

	r = usbi_backend->get_active_config_descriptor(dev, buf,	
		_config.wTotalLength, &host_endian);
	if (r >= 0)
		r = raw_desc_to_config(dev->ctx, buf, r, host_endian, config);

	free(buf);
	return r;
}


int API_EXPORTED libusb_get_config_descriptor(libusb_device *dev,
	uint8_t config_index, struct libusb_config_descriptor **config)
{
	struct libusb_config_descriptor _config;
	unsigned char tmp[LIBUSB_DT_CONFIG_SIZE];
	unsigned char *buf = NULL;
	int host_endian = 0;
	int r;

	usbi_dbg("index %d", config_index);
	if UNLIKELY(config_index >= dev->num_configurations)
		return LIBUSB_ERROR_NOT_FOUND;

	r = usbi_backend->get_config_descriptor(dev, config_index, tmp,
		LIBUSB_DT_CONFIG_SIZE, &host_endian);
	if UNLIKELY(r < 0)
		return r;
	if UNLIKELY(r < LIBUSB_DT_CONFIG_SIZE) {
		usbi_err(dev->ctx, "short config descriptor read %d/%d",
			 r, LIBUSB_DT_CONFIG_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(tmp, "bbw", &_config, host_endian);
	buf = malloc(_config.wTotalLength);
	if UNLIKELY(!buf)
		return LIBUSB_ERROR_NO_MEM;

	r = usbi_backend->get_config_descriptor(dev, config_index, buf,
		_config.wTotalLength, &host_endian);
	if LIKELY(r >= 0)
		r = raw_desc_to_config(dev->ctx, buf, r, host_endian, config);

	free(buf);
	return r;
}


int usbi_get_config_index_by_value(struct libusb_device *dev,
	uint8_t bConfigurationValue, int *idx)
{
	uint8_t i;

	usbi_dbg("value %d", bConfigurationValue);
	for (i = 0; i < dev->num_configurations; i++) {
		unsigned char tmp[6];
		int host_endian;
		int r = usbi_backend->get_config_descriptor(dev, i, tmp, sizeof(tmp),
			&host_endian);
		if UNLIKELY(r < 0) {
			*idx = -1;
			return r;
		}
		if (tmp[5] == bConfigurationValue) {
			*idx = i;
			return 0;
		}
	}

	*idx = -1;
	return 0;
}


int API_EXPORTED libusb_get_config_descriptor_by_value(libusb_device *dev,
	uint8_t bConfigurationValue, struct libusb_config_descriptor **config)
{
	int r, idx, host_endian;
	unsigned char *buf = NULL;

	if (usbi_backend->get_config_descriptor_by_value) {
		r = usbi_backend->get_config_descriptor_by_value(dev,
			bConfigurationValue, &buf, &host_endian);
		if UNLIKELY(r < 0)
			return r;
		return raw_desc_to_config(dev->ctx, buf, r, host_endian, config);
	}

	r = usbi_get_config_index_by_value(dev, bConfigurationValue, &idx);
	if UNLIKELY(r < 0)
		return r;
	else if UNLIKELY(idx == -1)
		return LIBUSB_ERROR_NOT_FOUND;
	else
		return libusb_get_config_descriptor(dev, (uint8_t) idx, config);
}


void API_EXPORTED libusb_free_config_descriptor(
	struct libusb_config_descriptor *config)
{
	if UNLIKELY(!config)
		return;

	clear_configuration(config);
	free(config);
}


int API_EXPORTED libusb_get_ss_endpoint_companion_descriptor(
	struct libusb_context *ctx,
	const struct libusb_endpoint_descriptor *endpoint,
	struct libusb_ss_endpoint_companion_descriptor **ep_comp)
{
	struct usb_descriptor_header header;
	int size = endpoint->extra_length;
	const unsigned char *buffer = endpoint->extra;

	*ep_comp = NULL;

	while (size >= LIBUSB_DT_HEADER_SIZE) {
		usbi_parse_descriptor(buffer, "bb", &header, 0);
		if UNLIKELY(header.bLength < 2 || header.bLength > size) {
			usbi_err(ctx, "invalid descriptor length %d",
				 header.bLength);
			return LIBUSB_ERROR_IO;
		}
		if (header.bDescriptorType != LIBUSB_DT_SS_ENDPOINT_COMPANION) {
			buffer += header.bLength;
			size -= header.bLength;
			continue;
		}
		if UNLIKELY(header.bLength < LIBUSB_DT_SS_ENDPOINT_COMPANION_SIZE) {
			usbi_err(ctx, "invalid ss-ep-comp-desc length %d",
				 header.bLength);
			return LIBUSB_ERROR_IO;
		}
		*ep_comp = malloc(sizeof(**ep_comp));
		if UNLIKELY(!*ep_comp)
			return LIBUSB_ERROR_NO_MEM;
		usbi_parse_descriptor(buffer, "bbbbw", *ep_comp, 0);
		return LIBUSB_SUCCESS;
	}
	return LIBUSB_ERROR_NOT_FOUND;
}


void API_EXPORTED libusb_free_ss_endpoint_companion_descriptor(
	struct libusb_ss_endpoint_companion_descriptor *ep_comp)
{
	free(ep_comp);
}

static int parse_bos(struct libusb_context *ctx,
	struct libusb_bos_descriptor **bos,
	unsigned char *buffer, int size, int host_endian)
{
	struct libusb_bos_descriptor bos_header, *_bos;
	struct libusb_bos_dev_capability_descriptor dev_cap;
	int i;

	if UNLIKELY(size < LIBUSB_DT_BOS_SIZE) {
		usbi_err(ctx, "short bos descriptor read %d/%d",
			 size, LIBUSB_DT_BOS_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(buffer, "bbwb", &bos_header, host_endian);
	if UNLIKELY(bos_header.bDescriptorType != LIBUSB_DT_BOS) {
		usbi_err(ctx, "unexpected descriptor %x (expected %x)",
			 bos_header.bDescriptorType, LIBUSB_DT_BOS);
		return LIBUSB_ERROR_IO;
	}
	if UNLIKELY(bos_header.bLength < LIBUSB_DT_BOS_SIZE) {
		usbi_err(ctx, "invalid bos bLength (%d)", bos_header.bLength);
		return LIBUSB_ERROR_IO;
	}
	if UNLIKELY(bos_header.bLength > size) {
		usbi_err(ctx, "short bos descriptor read %d/%d",
			 size, bos_header.bLength);
		return LIBUSB_ERROR_IO;
	}

	_bos = calloc (1,
		sizeof(*_bos) + bos_header.bNumDeviceCaps * sizeof(void *));
	if UNLIKELY(!_bos)
		return LIBUSB_ERROR_NO_MEM;

	usbi_parse_descriptor(buffer, "bbwb", _bos, host_endian);
	buffer += bos_header.bLength;
	size -= bos_header.bLength;

	
	for (i = 0; i < bos_header.bNumDeviceCaps; i++) {
		if (size < LIBUSB_DT_DEVICE_CAPABILITY_SIZE) {
			usbi_warn(ctx, "short dev-cap descriptor read %d/%d",
				  size, LIBUSB_DT_DEVICE_CAPABILITY_SIZE);
			break;
		}
		usbi_parse_descriptor(buffer, "bbb", &dev_cap, host_endian);
		if (dev_cap.bDescriptorType != LIBUSB_DT_DEVICE_CAPABILITY) {
			usbi_warn(ctx, "unexpected descriptor %x (expected %x)",
				  dev_cap.bDescriptorType, LIBUSB_DT_DEVICE_CAPABILITY);
			break;
		}
		if UNLIKELY(dev_cap.bLength < LIBUSB_DT_DEVICE_CAPABILITY_SIZE) {
			usbi_err(ctx, "invalid dev-cap bLength (%d)",
				 dev_cap.bLength);
			libusb_free_bos_descriptor(_bos);
			return LIBUSB_ERROR_IO;
		}
		if (dev_cap.bLength > size) {
			usbi_warn(ctx, "short dev-cap descriptor read %d/%d",
				  size, dev_cap.bLength);
			break;
		}

		_bos->dev_capability[i] = malloc(dev_cap.bLength);
		if UNLIKELY(!_bos->dev_capability[i]) {
			libusb_free_bos_descriptor(_bos);
			return LIBUSB_ERROR_NO_MEM;
		}
		memcpy(_bos->dev_capability[i], buffer, dev_cap.bLength);
		buffer += dev_cap.bLength;
		size -= dev_cap.bLength;
	}
	_bos->bNumDeviceCaps = (uint8_t)i;
	*bos = _bos;

	return LIBUSB_SUCCESS;
}


int API_EXPORTED libusb_get_bos_descriptor(libusb_device_handle *handle,
	struct libusb_bos_descriptor **bos)
{
	struct libusb_bos_descriptor _bos;
	uint8_t bos_header[LIBUSB_DT_BOS_SIZE] = {0};
	unsigned char *bos_data = NULL;
	const int host_endian = 0;
	int r;

	
	r = libusb_get_descriptor(handle, LIBUSB_DT_BOS, 0, bos_header,
				  LIBUSB_DT_BOS_SIZE);
	if UNLIKELY(r < 0) {
		if (r != LIBUSB_ERROR_PIPE)
			usbi_err(handle->dev->ctx, "failed to read BOS (%d)", r);
		return r;
	}
	if UNLIKELY(r < LIBUSB_DT_BOS_SIZE) {
		usbi_err(handle->dev->ctx, "short BOS read %d/%d",
			 r, LIBUSB_DT_BOS_SIZE);
		return LIBUSB_ERROR_IO;
	}

	usbi_parse_descriptor(bos_header, "bbwb", &_bos, host_endian);
	usbi_dbg("found BOS descriptor: size %d bytes, %d capabilities",
		 _bos.wTotalLength, _bos.bNumDeviceCaps);
	bos_data = calloc(_bos.wTotalLength, 1);
	if UNLIKELY(!bos_data)
		return LIBUSB_ERROR_NO_MEM;

	r = libusb_get_descriptor(handle, LIBUSB_DT_BOS, 0, bos_data,
				  _bos.wTotalLength);
	if LIKELY(r >= 0)
		r = parse_bos(handle->dev->ctx, bos, bos_data, r, host_endian);
	else
		usbi_err(handle->dev->ctx, "failed to read BOS (%d)", r);

	free(bos_data);
	return r;
}


void API_EXPORTED libusb_free_bos_descriptor(struct libusb_bos_descriptor *bos)
{
	int i;

	if (!bos)
		return;

	for (i = 0; i < bos->bNumDeviceCaps; i++)
		free(bos->dev_capability[i]);
	free(bos);
}


int API_EXPORTED libusb_get_usb_2_0_extension_descriptor(
	struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_usb_2_0_extension_descriptor **usb_2_0_extension)
{
	struct libusb_usb_2_0_extension_descriptor *_usb_2_0_extension;
	const int host_endian = 0;

	if UNLIKELY(dev_cap->bDevCapabilityType != LIBUSB_BT_USB_2_0_EXTENSION) {
		usbi_err(ctx, "unexpected bDevCapabilityType %x (expected %x)",
			 dev_cap->bDevCapabilityType,
			 LIBUSB_BT_USB_2_0_EXTENSION);
		return LIBUSB_ERROR_INVALID_PARAM;
	}
	if UNLIKELY(dev_cap->bLength < LIBUSB_BT_USB_2_0_EXTENSION_SIZE) {
		usbi_err(ctx, "short dev-cap descriptor read %d/%d",
			 dev_cap->bLength, LIBUSB_BT_USB_2_0_EXTENSION_SIZE);
		return LIBUSB_ERROR_IO;
	}

	_usb_2_0_extension = malloc(sizeof(*_usb_2_0_extension));
	if UNLIKELY(!_usb_2_0_extension)
		return LIBUSB_ERROR_NO_MEM;

	usbi_parse_descriptor((unsigned char *)dev_cap, "bbbd",
			      _usb_2_0_extension, host_endian);

	*usb_2_0_extension = _usb_2_0_extension;
	return LIBUSB_SUCCESS;
}


void API_EXPORTED libusb_free_usb_2_0_extension_descriptor(
	struct libusb_usb_2_0_extension_descriptor *usb_2_0_extension)
{
	free(usb_2_0_extension);
}


int API_EXPORTED libusb_get_ss_usb_device_capability_descriptor(
	struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_ss_usb_device_capability_descriptor **ss_usb_device_cap)
{
	struct libusb_ss_usb_device_capability_descriptor *_ss_usb_device_cap;
	const int host_endian = 0;

	if UNLIKELY(dev_cap->bDevCapabilityType != LIBUSB_BT_SS_USB_DEVICE_CAPABILITY) {
		usbi_err(ctx, "unexpected bDevCapabilityType %x (expected %x)",
			 dev_cap->bDevCapabilityType,
			 LIBUSB_BT_SS_USB_DEVICE_CAPABILITY);
		return LIBUSB_ERROR_INVALID_PARAM;
	}
	if UNLIKELY(dev_cap->bLength < LIBUSB_BT_SS_USB_DEVICE_CAPABILITY_SIZE) {
		usbi_err(ctx, "short dev-cap descriptor read %d/%d",
			 dev_cap->bLength, LIBUSB_BT_SS_USB_DEVICE_CAPABILITY_SIZE);
		return LIBUSB_ERROR_IO;
	}

	_ss_usb_device_cap = malloc(sizeof(*_ss_usb_device_cap));
	if UNLIKELY(!_ss_usb_device_cap)
		return LIBUSB_ERROR_NO_MEM;

	usbi_parse_descriptor((unsigned char *)dev_cap, "bbbbwbbw",
			      _ss_usb_device_cap, host_endian);

	*ss_usb_device_cap = _ss_usb_device_cap;
	return LIBUSB_SUCCESS;
}


void API_EXPORTED libusb_free_ss_usb_device_capability_descriptor(
	struct libusb_ss_usb_device_capability_descriptor *ss_usb_device_cap)
{
	free(ss_usb_device_cap);
}


int API_EXPORTED libusb_get_container_id_descriptor(struct libusb_context *ctx,
	struct libusb_bos_dev_capability_descriptor *dev_cap,
	struct libusb_container_id_descriptor **container_id)
{
	struct libusb_container_id_descriptor *_container_id;
	const int host_endian = 0;

	if UNLIKELY(dev_cap->bDevCapabilityType != LIBUSB_BT_CONTAINER_ID) {
		usbi_err(ctx, "unexpected bDevCapabilityType %x (expected %x)",
			 dev_cap->bDevCapabilityType,
			 LIBUSB_BT_CONTAINER_ID);
		return LIBUSB_ERROR_INVALID_PARAM;
	}
	if UNLIKELY(dev_cap->bLength < LIBUSB_BT_CONTAINER_ID_SIZE) {
		usbi_err(ctx, "short dev-cap descriptor read %d/%d",
			 dev_cap->bLength, LIBUSB_BT_CONTAINER_ID_SIZE);
		return LIBUSB_ERROR_IO;
	}

	_container_id = malloc(sizeof(*_container_id));
	if UNLIKELY(!_container_id)
		return LIBUSB_ERROR_NO_MEM;

	usbi_parse_descriptor((unsigned char *)dev_cap, "bbbbu",
			      _container_id, host_endian);

	*container_id = _container_id;
	return LIBUSB_SUCCESS;
}


void API_EXPORTED libusb_free_container_id_descriptor(
	struct libusb_container_id_descriptor *container_id)
{
	free(container_id);
}


int API_EXPORTED libusb_get_string_descriptor_ascii(libusb_device_handle *dev,
	uint8_t desc_index, unsigned char *data, int length)
{
	unsigned char tbuf[255]; 
	int r, si, di;
	uint16_t langid;

	

	if UNLIKELY(!desc_index)
		return LIBUSB_ERROR_INVALID_PARAM;

	r = libusb_get_string_descriptor(dev, 0, 0, tbuf, sizeof(tbuf));
	if UNLIKELY(r < 0)
		return r;

	if UNLIKELY(r < 4)
		return LIBUSB_ERROR_IO;

	langid = tbuf[2] | (tbuf[3] << 8);

	r = libusb_get_string_descriptor(dev, desc_index, langid, tbuf, sizeof(tbuf));
	if UNLIKELY(r < 0)
		return r;

	if UNLIKELY(tbuf[1] != LIBUSB_DT_STRING)
		return LIBUSB_ERROR_IO;

	if UNLIKELY(tbuf[0] > r)
		return LIBUSB_ERROR_IO;

	for (di = 0, si = 2; si < tbuf[0]; si += 2) {
		if (di >= (length - 1))
			break;

		if ((tbuf[si] & 0x80) || (tbuf[si + 1])) 
			data[di++] = '?';
		else
			data[di++] = tbuf[si];
	}

	data[di] = 0;
	return di;
}
