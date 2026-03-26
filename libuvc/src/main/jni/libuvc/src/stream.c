




#define LOCAL_DEBUG 0

#define LOG_TAG "libuvc/stream"
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

#include <assert.h>		

#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"
#include <errno.h>

uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
		uint16_t format_id, uint16_t frame_id);
uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
		uint16_t format_id, uint16_t frame_id);
static void *_uvc_user_caller(void *arg);
static void _uvc_populate_frame(uvc_stream_handle_t *strmh);

struct format_table_entry {
	enum uvc_frame_format format;
	uint8_t abstract_fmt;
	uint8_t guid[16];
	int children_count;
	enum uvc_frame_format *children;
};

struct timespec ts;
struct timeval tv;

struct format_table_entry *_get_format_entry(enum uvc_frame_format format) {
#define ABS_FMT(_fmt, ...) \
    case _fmt: { \
    static enum uvc_frame_format _fmt##_children[] = __VA_ARGS__; \
    static struct format_table_entry _fmt##_entry = { \
      _fmt, 0, {}, ARRAYSIZE(_fmt##_children), _fmt##_children }; \
    return &_fmt##_entry; }

#define FMT(_fmt, ...) \
    case _fmt: { \
    static struct format_table_entry _fmt##_entry = { \
      _fmt, 0, __VA_ARGS__, 0, NULL }; \
    return &_fmt##_entry; }

	switch (format) {
	
	ABS_FMT(UVC_FRAME_FORMAT_ANY,
		{UVC_FRAME_FORMAT_UNCOMPRESSED, UVC_FRAME_FORMAT_COMPRESSED})

	ABS_FMT(UVC_FRAME_FORMAT_UNCOMPRESSED,
		{UVC_FRAME_FORMAT_YUYV, UVC_FRAME_FORMAT_UYVY, UVC_FRAME_FORMAT_GRAY8})
	FMT(UVC_FRAME_FORMAT_YUYV,
		{'Y', 'U', 'Y', '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
	FMT(UVC_FRAME_FORMAT_UYVY,
		{'U', 'Y', 'V', 'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
	FMT(UVC_FRAME_FORMAT_GRAY8,
		{'Y', '8', '0', '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_BY8,
    	{'B', 'Y', '8', ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})

	ABS_FMT(UVC_FRAME_FORMAT_COMPRESSED,
		{UVC_FRAME_FORMAT_MJPEG})
	FMT(UVC_FRAME_FORMAT_MJPEG,
		{'M', 'J', 'P', 'G'})

	default:
		return NULL;
	}

#undef ABS_FMT
#undef FMT
}

static uint8_t _uvc_frame_format_matches_guid(enum uvc_frame_format fmt,
		uint8_t guid[16]) {
	struct format_table_entry *format;
	int child_idx;

	format = _get_format_entry(fmt);
	if (UNLIKELY(!format))
		return 0;

	if (!format->abstract_fmt && !memcmp(guid, format->guid, 16))
		return 1;

	for (child_idx = 0; child_idx < format->children_count; child_idx++) {
		if (_uvc_frame_format_matches_guid(format->children[child_idx], guid))
			return 1;
	}

	return 0;
}

static enum uvc_frame_format uvc_frame_format_for_guid(uint8_t guid[16]) {
	struct format_table_entry *format;
	enum uvc_frame_format fmt;

	for (fmt = 0; fmt < UVC_FRAME_FORMAT_COUNT; ++fmt) {
		format = _get_format_entry(fmt);
		if (!format || format->abstract_fmt)
			continue;
		if (!memcmp(format->guid, guid, 16))
			return format->format;
	}

	return UVC_FRAME_FORMAT_UNKNOWN;
}


uvc_error_t uvc_query_stream_ctrl(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uint8_t probe, enum uvc_req_code req) {
	uint8_t buf[48];	
	size_t len;
	uvc_error_t err;

	memset(buf, 0, sizeof(buf));	

	const uint16_t bcdUVC = devh->info->ctrl_if.bcdUVC;
	if (bcdUVC >= 0x0150)
		len = 48;
	else if (bcdUVC >= 0x0110)
		len = 34;
	else
		len = 26;

	
	if (req == UVC_SET_CUR) {
		SHORT_TO_SW(ctrl->bmHint, buf);
		buf[2] = ctrl->bFormatIndex;
		buf[3] = ctrl->bFrameIndex;
		INT_TO_DW(ctrl->dwFrameInterval, buf + 4);
		SHORT_TO_SW(ctrl->wKeyFrameRate, buf + 8);
		SHORT_TO_SW(ctrl->wPFrameRate, buf + 10);
		SHORT_TO_SW(ctrl->wCompQuality, buf + 12);
		SHORT_TO_SW(ctrl->wCompWindowSize, buf + 14);
		SHORT_TO_SW(ctrl->wDelay, buf + 16);
		INT_TO_DW(ctrl->dwMaxVideoFrameSize, buf + 18);
		INT_TO_DW(ctrl->dwMaxPayloadTransferSize, buf + 22);

		if (len > 26) {	
			
			INT_TO_DW(ctrl->dwClockFrequency, buf + 26);
			buf[30] = ctrl->bmFramingInfo;
			buf[31] = ctrl->bPreferedVersion;
			buf[32] = ctrl->bMinVersion;
			buf[33] = ctrl->bMaxVersion;
			if (len == 48) {
				
				buf[34] = ctrl->bUsage;
				buf[35] = ctrl->bBitDepthLuma;
				buf[36] = ctrl->bmSettings;
				buf[37] = ctrl->bMaxNumberOfRefFramesPlus1;
				SHORT_TO_SW(ctrl->bmRateControlModes, buf + 38);
				LONG_TO_QW(ctrl->bmLayoutPerStream, buf + 40);
			}
		}
	}

	
	err = libusb_control_transfer(devh->usb_devh,
			req == UVC_SET_CUR ? 0x21 : 0xA1, req,
			probe ? (UVC_VS_PROBE_CONTROL << 8) : (UVC_VS_COMMIT_CONTROL << 8),
			ctrl->bInterfaceNumber, buf, len, 0);

	if (UNLIKELY(err <= 0)) {
		
		if (!err) {
			UVC_DEBUG("libusb_control_transfer transfered zero length data");
			err = UVC_ERROR_OTHER;
		}
		return err;
	}
	if (err < len) {
#if !defined(__LP64__)
		LOGE("transfered bytes is smaller than data bytes:%d expected %d", err, len);
#else
		LOGE("transfered bytes is smaller than data bytes:%d expected %ld", err, len);
#endif
		return UVC_ERROR_OTHER;
	}
	
	if (req != UVC_SET_CUR) {
		ctrl->bmHint = SW_TO_SHORT(buf);
		ctrl->bFormatIndex = buf[2];
		ctrl->bFrameIndex = buf[3];
		ctrl->dwFrameInterval = DW_TO_INT(buf + 4);
		ctrl->wKeyFrameRate = SW_TO_SHORT(buf + 8);
		ctrl->wPFrameRate = SW_TO_SHORT(buf + 10);
		ctrl->wCompQuality = SW_TO_SHORT(buf + 12);
		ctrl->wCompWindowSize = SW_TO_SHORT(buf + 14);
		ctrl->wDelay = SW_TO_SHORT(buf + 16);
		ctrl->dwMaxVideoFrameSize = DW_TO_INT(buf + 18);
		ctrl->dwMaxPayloadTransferSize = DW_TO_INT(buf + 22);

		if (len > 26) {	
			
			ctrl->dwClockFrequency = DW_TO_INT(buf + 26);
			ctrl->bmFramingInfo = buf[30];
			ctrl->bPreferedVersion = buf[31];
			ctrl->bMinVersion = buf[32];
			ctrl->bMaxVersion = buf[33];
			if (len >= 48) {
				
				ctrl->bUsage = buf[34];
				ctrl->bBitDepthLuma = buf[35];
				ctrl->bmSettings = buf[36];
				ctrl->bMaxNumberOfRefFramesPlus1 = buf[37];
				ctrl->bmRateControlModes = SW_TO_SHORT(buf + 38);
				ctrl->bmLayoutPerStream = QW_TO_LONG(buf + 40);
			}
		}

		
		if (!ctrl->dwMaxVideoFrameSize) {
			LOGW("fix up block for cameras that fail to set dwMax");
			uvc_frame_desc_t *frame_desc = uvc_find_frame_desc(devh,
					ctrl->bFormatIndex, ctrl->bFrameIndex);

			if (frame_desc) {
				ctrl->dwMaxVideoFrameSize = frame_desc->dwMaxVideoFrameBufferSize;
			}
		}
	}

	return UVC_SUCCESS;
}


uvc_error_t uvc_stream_ctrl(uvc_stream_handle_t *strmh, uvc_stream_ctrl_t *ctrl) {
	uvc_error_t ret;

	if (UNLIKELY(strmh->stream_if->bInterfaceNumber != ctrl->bInterfaceNumber))
		return UVC_ERROR_INVALID_PARAM;

	
	if (UNLIKELY(strmh->running))
		return UVC_ERROR_BUSY;

	ret = uvc_query_stream_ctrl(strmh->devh, ctrl, 0, UVC_SET_CUR);	
	if (UNLIKELY(ret != UVC_SUCCESS))
		return ret;

	strmh->cur_ctrl = *ctrl;
	return UVC_SUCCESS;
}


static uvc_frame_desc_t *_uvc_find_frame_desc_stream_if(
		uvc_streaming_interface_t *stream_if, uint16_t format_id,
		uint16_t frame_id) {

	uvc_format_desc_t *format = NULL;
	uvc_frame_desc_t *frame = NULL;

	DL_FOREACH(stream_if->format_descs, format)
	{
		if (format->bFormatIndex == format_id) {
			DL_FOREACH(format->frame_descs, frame)
			{
				if (frame->bFrameIndex == frame_id)
					return frame;
			}
		}
	}

	return NULL ;
}

uvc_error_t uvc_get_frame_desc(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_desc_t **desc) {

	*desc = uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);
	return *desc ? UVC_SUCCESS : UVC_ERROR_INVALID_PARAM;
}

uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
		uint16_t format_id, uint16_t frame_id) {
	return _uvc_find_frame_desc_stream_if(strmh->stream_if, format_id, frame_id);
}


uvc_frame_desc_t *uvc_find_frame_desc(uvc_device_handle_t *devh,
		uint16_t format_id, uint16_t frame_id) {

	uvc_streaming_interface_t *stream_if;
	uvc_frame_desc_t *frame;

	DL_FOREACH(devh->info->stream_ifs, stream_if)
	{
		frame = _uvc_find_frame_desc_stream_if(stream_if, format_id, frame_id);
		if (frame)
			return frame;
	}

	return NULL;
}

static void _uvc_print_streaming_interface_one(uvc_streaming_interface_t *stream_if) {


	MARK("bInterfaceNumber:%d", stream_if->bInterfaceNumber);
	uvc_print_format_desc_one(stream_if->format_descs, NULL);
	MARK("bEndpointAddress:%d", stream_if->bEndpointAddress);
	MARK("bTerminalLink:%d", stream_if->bTerminalLink);
}

static uvc_error_t _prepare_stream_ctrl(uvc_device_handle_t *devh, uvc_stream_ctrl_t *ctrl) {
	
	
	uvc_error_t result = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_CUR);	
	if (LIKELY(!result)) {
		result = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MIN);			
		if (LIKELY(!result)) {
			result = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);		
			if (UNLIKELY(result))
				LOGE("uvc_query_stream_ctrl:UVC_GET_MAX:err=%d", result);	
		} else {
			LOGE("uvc_query_stream_ctrl:UVC_GET_MIN:err=%d", result);
		}
	} else {
		LOGE("uvc_query_stream_ctrl:UVC_GET_CUR:err=%d", result);
	}
#if 0
	if (UNLIKELY(result)) {
		enum uvc_error_code_control error_code;
		uvc_get_error_code(devh, &error_code, UVC_GET_CUR);
		LOGE("uvc_query_stream_ctrl:ret=%d,err_code=%d", result, error_code);
		uvc_print_format_desc(devh->info->stream_ifs->format_descs, NULL);
	}
#endif
	return result;
}

static uvc_error_t _uvc_get_stream_ctrl_format(uvc_device_handle_t *devh,
	uvc_streaming_interface_t *stream_if, uvc_stream_ctrl_t *ctrl, uvc_format_desc_t *format,
	const int width, const int height,
	const int min_fps, const int max_fps) {

	ENTER();

	int i;
	uvc_frame_desc_t *frame;

	ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
	uvc_error_t result = uvc_claim_if(devh, ctrl->bInterfaceNumber);
	if (UNLIKELY(result)) {
		LOGE("uvc_claim_if:err=%d", result);
		goto fail;
	}
	for (i = 0; i < 2; i++) {
		result = _prepare_stream_ctrl(devh, ctrl);
	}
	if (UNLIKELY(result)) {
		LOGE("_prepare_stream_ctrl:err=%d", result);
		goto fail;
	}
#if 0
	
	uint64_t bmaControl = stream_if->bmaControls[format->bFormatIndex - 1];
	if (bmaControl & 0x001) {	
		if (UNLIKELY(!ctrl->wKeyFrameRate)) {
			LOGE("wKeyFrameRate should be set");
			RETURN(UVC_ERROR_INVALID_MODE, uvc_error_t);
		}
	}
	if (bmaControl & 0x002) {	
		if (UNLIKELY(!ctrl->wPFrameRate)) {
			LOGE("wPFrameRate should be set");
			RETURN(UVC_ERROR_INVALID_MODE, uvc_error_t);
		}
	}
	if (bmaControl & 0x004) {	
		if (UNLIKELY(!ctrl->wCompQuality)) {
			LOGE("wCompQuality should be set");
			RETURN(UVC_ERROR_INVALID_MODE, uvc_error_t);
		}
	}
	if (bmaControl & 0x008) {	
		if (UNLIKELY(!ctrl->wCompWindowSize)) {
			LOGE("wCompWindowSize should be set");
			RETURN(UVC_ERROR_INVALID_MODE, uvc_error_t);
		}
	}
#endif
	DL_FOREACH(format->frame_descs, frame)
	{
		if (frame->wWidth != width || frame->wHeight != height)
			continue;

		uint32_t *interval;

		if (frame->intervals) {
			
			uint32_t best_interval = 0;
			uint32_t best_fps = 0;
			for (interval = frame->intervals; *interval; ++interval) {
				if (UNLIKELY(!(*interval))) continue;
				uint32_t it = 10000000 / *interval;
				LOGV("it:%d", it);
				if ((it >= (uint32_t) min_fps) && (it <= (uint32_t) max_fps)
						&& it > best_fps) {
					best_fps = it;
					best_interval = *interval;
				}
			}
			if (best_interval) {
				ctrl->bmHint = (1 << 0); 
				ctrl->bFormatIndex = format->bFormatIndex;
				ctrl->bFrameIndex = frame->bFrameIndex;
				ctrl->dwFrameInterval = best_interval;

				goto found;
			}
		} else {
			int32_t fps;
			for (fps = max_fps; fps >= min_fps; fps--) {
				if (UNLIKELY(!fps)) continue;
				uint32_t interval_100ns = 10000000 / fps;
				uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;
				LOGV("fps:%d", fps);
				if (interval_100ns >= frame->dwMinFrameInterval
					&& interval_100ns <= frame->dwMaxFrameInterval
					&& !(interval_offset
						&& (interval_offset % frame->dwFrameIntervalStep) ) ) {
					ctrl->bmHint = (1 << 0); 
					ctrl->bFormatIndex = format->bFormatIndex;
					ctrl->bFrameIndex = frame->bFrameIndex;
					ctrl->dwFrameInterval = interval_100ns;

					goto found;
				}
			}
		}
	}
	result = UVC_ERROR_INVALID_MODE;
fail:
	uvc_release_if(devh, ctrl->bInterfaceNumber);
	RETURN(result, uvc_error_t);

found:
	RETURN(UVC_SUCCESS, uvc_error_t);
}


uvc_error_t uvc_get_stream_ctrl_format_size(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, enum uvc_frame_format cf, int width, int height, int fps) {

	return uvc_get_stream_ctrl_format_size_fps(devh, ctrl, cf, width, height, fps, fps);
}


uvc_error_t uvc_get_stream_ctrl_format_size_fps(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, enum uvc_frame_format cf, int width,
		int height, int min_fps, int max_fps) {

	ENTER();

	uvc_streaming_interface_t *stream_if;
	uvc_error_t result;

	memset(ctrl, 0, sizeof(*ctrl));	
	
	uvc_format_desc_t *format;
	DL_FOREACH(devh->info->stream_ifs, stream_if)
	{
		DL_FOREACH(stream_if->format_descs, format)
		{
			if (!_uvc_frame_format_matches_guid(cf, format->guidFormat))
				continue;

			result = _uvc_get_stream_ctrl_format(devh, stream_if, ctrl, format, width, height, min_fps, max_fps);
			if (!result) {	
				goto found;
			}
		}
	}

	RETURN(UVC_ERROR_INVALID_MODE, uvc_error_t);

found:
	RETURN(uvc_probe_stream_ctrl(devh, ctrl), uvc_error_t);
}


uvc_error_t uvc_probe_stream_ctrl(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl) {
	uvc_error_t err;

	err = uvc_claim_if(devh, ctrl->bInterfaceNumber);
	if (UNLIKELY(err)) {
		LOGE("uvc_claim_if:err=%d", err);
		return err;
	}

	err = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_SET_CUR);	
	if (UNLIKELY(err)) {
		LOGE("uvc_query_stream_ctrl(UVC_SET_CUR):err=%d", err);
		return err;
	}

	err = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_CUR);	
	if (UNLIKELY(err)) {
		LOGE("uvc_query_stream_ctrl(UVC_GET_CUR):err=%d", err);
		return err;
	}

	return UVC_SUCCESS;
}


static void _uvc_swap_buffers(uvc_stream_handle_t *strmh) {
	uint8_t *tmp_buf;

	pthread_mutex_lock(&strmh->cb_mutex);
	{
		
		tmp_buf = strmh->holdbuf;
		strmh->hold_bfh_err = strmh->bfh_err;	
		strmh->hold_bytes = strmh->got_bytes;
		strmh->holdbuf = strmh->outbuf;
		strmh->outbuf = tmp_buf;
		strmh->hold_last_scr = strmh->last_scr;
		strmh->hold_pts = strmh->pts;
		strmh->hold_seq = strmh->seq;

		pthread_cond_broadcast(&strmh->cb_cond);
	}
	pthread_mutex_unlock(&strmh->cb_mutex);

	strmh->seq++;
	strmh->got_bytes = 0;
	strmh->last_scr = 0;
	strmh->pts = 0;
	strmh->bfh_err = 0;	
}

static void _uvc_delete_transfer(struct libusb_transfer *transfer) {
	ENTER();


	uvc_stream_handle_t *strmh = transfer->user_data;
	if (UNLIKELY(!strmh)) EXIT();		
	int i;

	pthread_mutex_lock(&strmh->cb_mutex);	
	{
		
		for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++) {
			if (strmh->transfers[i] == transfer) {
				libusb_cancel_transfer(strmh->transfers[i]);	
				UVC_DEBUG("Freeing transfer %d (%p)", i, transfer);
				free(transfer->buffer);
				
				strmh->transfers[i] = NULL;
				break;
			}
		}
		if (UNLIKELY(i == LIBUVC_NUM_TRANSFER_BUFS)) {
			UVC_DEBUG("transfer %p not found; not freeing!", transfer);
		}

		pthread_cond_broadcast(&strmh->cb_cond);
	}
	pthread_mutex_unlock(&strmh->cb_mutex);
	EXIT();
}

#define USE_EOF


static void _uvc_process_payload(uvc_stream_handle_t *strmh, const uint8_t *payload, size_t const payload_len) {
	size_t header_len;
	uint8_t header_info;
	size_t data_len;
	struct libusb_iso_packet_descriptor *pkt;
	uvc_vc_error_code_control_t vc_error_code;
	uvc_vs_error_code_control_t vs_error_code;

	
	static const uint8_t isight_tag[] = {
		0x11, 0x22, 0x33, 0x44,
		0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce
	};

	
	if (UNLIKELY(!payload || !payload_len || !strmh->outbuf))
		return;

	

	if (UNLIKELY(strmh->devh->is_isight &&
		((payload_len < 14) || memcmp(isight_tag, payload + 2, sizeof(isight_tag)) ) &&
		((payload_len < 15) || memcmp(isight_tag, payload + 3, sizeof(isight_tag)) ) )) {
		
		header_len = 0;
		data_len = payload_len;
	} else {
		header_len = payload[0];

		if (UNLIKELY(header_len > payload_len)) {
			strmh->bfh_err |= UVC_STREAM_ERR;
			UVC_DEBUG("bogus packet: actual_len=%zd, header_len=%zd\n", payload_len, header_len);
			return;
		}

		if (UNLIKELY(strmh->devh->is_isight))
			data_len = 0;
		else
			data_len = payload_len - header_len;
	}

	if (UNLIKELY(header_len < 2)) {
		header_info = 0;
	} else {
		
		size_t variable_offset = 2;

		header_info = payload[1];

		if (UNLIKELY(header_info & UVC_STREAM_ERR)) {

			UVC_DEBUG("bad packet: error bit set");
			libusb_clear_halt(strmh->devh->usb_devh, strmh->stream_if->bEndpointAddress);

			uvc_vs_get_error_code(strmh->devh, &vs_error_code, UVC_GET_CUR);

		}

		if ((strmh->fid != (header_info & UVC_STREAM_FID)) && strmh->got_bytes) {
			
			_uvc_swap_buffers(strmh);
		}

		strmh->fid = header_info & UVC_STREAM_FID;

		if (header_info & UVC_STREAM_PTS) {
			
			if (LIKELY(variable_offset + 4 <= header_len)) {
				strmh->pts = DW_TO_INT(payload + variable_offset);
				variable_offset += 4;
			} else {
				MARK("bogus packet: header info has UVC_STREAM_PTS, but no data");
				strmh->pts = 0;
			}
		}

		if (header_info & UVC_STREAM_SCR) {
			
			
			if (LIKELY(variable_offset + 4 <= header_len)) {
				strmh->last_scr = DW_TO_INT(payload + variable_offset);
				variable_offset += 4;
			} else {
				MARK("bogus packet: header info has UVC_STREAM_SCR, but no data");
				strmh->last_scr = 0;
			}
		}
	}

	if (LIKELY(data_len > 0)) {
		if (LIKELY(strmh->got_bytes + data_len < strmh->size_buf)) {
			memcpy(strmh->outbuf + strmh->got_bytes, payload + header_len, data_len);
			strmh->got_bytes += data_len;
		} else {
			strmh->bfh_err |= UVC_STREAM_ERR;
		}

		if (header_info & UVC_STREAM_EOF) {
			
			_uvc_swap_buffers(strmh);
		}
	}
}

#if 0
static inline void _uvc_process_payload_iso(uvc_stream_handle_t *strmh, struct libusb_transfer *transfer) {
	
	int packet_id;
	for (packet_id = 0; packet_id < transfer->num_iso_packets; packet_id++) {
		struct libusb_iso_packet_descriptor *pkt = transfer->iso_packet_desc + packet_id;

		if UNLIKELY(pkt->status) {

			MARK("bad packet:status=%d,actual_length=%d", pkt->status, pkt->actual_length);
			continue;
		}
		if UNLIKELY(!pkt->actual_length) {
			MARK("zero packet (transfer):");
			continue;
		}
		
		uint8_t *pktbuf = libusb_get_iso_packet_buffer_simple(transfer, packet_id);
		_uvc_process_payload(strmh, pktbuf, pkt->actual_length);
	}
}
#else
static inline void _uvc_process_payload_iso(uvc_stream_handle_t *strmh, struct libusb_transfer *transfer) {
	
	uint8_t *pktbuf;
	uint8_t check_header;
	size_t header_len;
	uint8_t header_info;
	struct libusb_iso_packet_descriptor *pkt;

	
	static const uint8_t isight_tag[] = {
		0x11, 0x22, 0x33, 0x44, 0xde, 0xad,
		0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce };
	int packet_id;
	uvc_vc_error_code_control_t vc_error_code;
	uvc_vs_error_code_control_t vs_error_code;

	for (packet_id = 0; packet_id < transfer->num_iso_packets; ++packet_id) {
		check_header = 1;

		pkt = transfer->iso_packet_desc + packet_id;

		if (UNLIKELY(pkt->status != 0)) {
			MARK("bad packet:status=%d,actual_length=%d", pkt->status, pkt->actual_length);
			strmh->bfh_err |= UVC_STREAM_ERR;
			libusb_clear_halt(strmh->devh->usb_devh, strmh->stream_if->bEndpointAddress);


			continue;
		}

		if (UNLIKELY(!pkt->actual_length)) {	


			continue;
		}
		
		
		pktbuf = libusb_get_iso_packet_buffer_simple(transfer, packet_id);
		if (LIKELY(pktbuf)) {	

#ifdef __ANDROID__
			
			if (UNLIKELY(strmh->devh->is_isight))
#else
			if (strmh->devh->is_isight)
#endif
			{
				if (pkt->actual_length < 30
					|| (memcmp(isight_tag, pktbuf + 2, sizeof(isight_tag))
						&& memcmp(isight_tag, pktbuf + 3, sizeof(isight_tag)))) {
					check_header = 0;
					header_len = 0;
				} else {
					header_len = pktbuf[0];
				}
			} else {
				header_len = pktbuf[0];	
			}

			if (LIKELY(check_header)) {
				header_info = pktbuf[1];
				if (UNLIKELY(header_info & UVC_STREAM_ERR)) {

					MARK("bad packet:status=0x%2x", header_info);
					libusb_clear_halt(strmh->devh->usb_devh, strmh->stream_if->bEndpointAddress);

					uvc_vs_get_error_code(strmh->devh, &vs_error_code, UVC_GET_CUR);
					continue;
				}
#ifdef USE_EOF
				if ((strmh->fid != (header_info & UVC_STREAM_FID)) && strmh->got_bytes) {	
				
					_uvc_swap_buffers(strmh);
				}
				strmh->fid = header_info & UVC_STREAM_FID;
#else
				if (strmh->fid != (header_info & UVC_STREAM_FID)) {	
					_uvc_swap_buffers(strmh);
					strmh->fid = header_info & UVC_STREAM_FID;
				}
#endif
				if (header_info & UVC_STREAM_PTS) {
					
					if (LIKELY(header_len >= 6)) {
						strmh->pts = DW_TO_INT(pktbuf + 2);
					} else {
						MARK("bogus packet: header info has UVC_STREAM_PTS, but no data");
						strmh->pts = 0;
					}
				}

				if (header_info & UVC_STREAM_SCR) {
					
					if (LIKELY(header_len >= 10)) {
						strmh->last_scr = DW_TO_INT(pktbuf + 6);
					} else {
						MARK("bogus packet: header info has UVC_STREAM_SCR, but no data");
						strmh->last_scr = 0;
					}
				}

#ifdef __ANDROID__	
				if (UNLIKELY(strmh->devh->is_isight))
					continue; 
#else
				if (strmh->devh->is_isight) {
					MARK("is_isight");
					continue; 
				}
#endif
			} 

			if (UNLIKELY(pkt->actual_length < header_len)) {
				
				strmh->bfh_err |= UVC_STREAM_ERR;
				MARK("bogus packet: actual_len=%d, header_len=%zd", pkt->actual_length, header_len);
				continue;
			}

			
			
			
			
			if (LIKELY(pkt->actual_length > header_len)) {
				const size_t odd_bytes = pkt->actual_length - header_len;
				assert(strmh->got_bytes + odd_bytes < strmh->size_buf);
				assert(strmh->outbuf);
				assert(pktbuf);
				memcpy(strmh->outbuf + strmh->got_bytes, pktbuf + header_len, odd_bytes);
				strmh->got_bytes += odd_bytes;
			}
#ifdef USE_EOF
			if ((pktbuf[1] & UVC_STREAM_EOF) && strmh->got_bytes != 0) {
				
				_uvc_swap_buffers(strmh);
			}
#endif
		} else {	
			strmh->bfh_err |= UVC_STREAM_ERR;
			MARK("libusb_get_iso_packet_buffer_simple returned null");
			continue;
		}
	}	
}
#endif


static void _uvc_stream_callback(struct libusb_transfer *transfer) {
	if UNLIKELY(!transfer) return;

	uvc_stream_handle_t *strmh = transfer->user_data;
	if UNLIKELY(!strmh) return;

	int resubmit = 1;

#ifndef NDEBUG
	static int cnt = 0;
	if UNLIKELY((++cnt % 1000) == 0)
		MARK("cnt=%d", cnt);
#endif
	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		if (!transfer->num_iso_packets) {
			
			_uvc_process_payload(strmh, transfer->buffer, transfer->actual_length);
		} else {
			
			_uvc_process_payload_iso(strmh, transfer);
		}
	    break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		strmh->running = 0;	
		
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_ERROR:
		libusb_clear_halt(strmh->devh->usb_devh, strmh->stream_if->bEndpointAddress);
		UVC_DEBUG("not retrying transfer, status = %d", transfer->status);


		resubmit = 0;
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_OVERFLOW:
		UVC_DEBUG("retrying transfer, status = %d", transfer->status);

		break;
	}

	if (LIKELY(strmh->running && resubmit)) {
		libusb_submit_transfer(transfer);
	} else {
		
		
		_uvc_delete_transfer(transfer);
	}
}

#if 0

static void _uvc_iso_callback(struct libusb_transfer *transfer) {
	uvc_stream_handle_t *strmh;
	int packet_id;

	
	uint8_t *pktbuf;
	uint8_t check_header;
	size_t header_len;	
	uint8_t header_info;
	struct libusb_iso_packet_descriptor *pkt;

	
	static const uint8_t isight_tag[] = {
		0x11, 0x22, 0x33, 0x44, 0xde, 0xad,
		0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce };

	strmh = transfer->user_data;
#ifndef NDEBUG
	static int cnt = 0;
	if ((++cnt % 1000) == 0)
		MARK("cnt=%d", cnt);
#endif
	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		if (UNLIKELY(!transfer->num_iso_packets))
			MARK("num_iso_packets is zero");
		for (packet_id = 0; packet_id < transfer->num_iso_packets; ++packet_id) {
			check_header = 1;

			pkt = transfer->iso_packet_desc + packet_id;

			if (UNLIKELY(pkt->status != 0)) {
				MARK("bad packet:status=%d,actual_length=%d", pkt->status, pkt->actual_length);
				strmh->bfh_err |= UVC_STREAM_ERR;
				continue;
			}

			if (UNLIKELY(!pkt->actual_length)) {	


				continue;
			}
			
			
			pktbuf = libusb_get_iso_packet_buffer_simple(transfer, packet_id);
			if (LIKELY(pktbuf)) {	

#ifdef __ANDROID__
				
				if (UNLIKELY(strmh->devh->is_isight))
#else
				if (strmh->devh->is_isight)
#endif
				{
					if (pkt->actual_length < 30
						|| (memcmp(isight_tag, pktbuf + 2, sizeof(isight_tag))
							&& memcmp(isight_tag, pktbuf + 3, sizeof(isight_tag)))) {
						check_header = 0;
						header_len = 0;
					} else {
						header_len = pktbuf[0];
					}
				} else {
					header_len = pktbuf[0];	
				}

				if (LIKELY(check_header)) {
					header_info = pktbuf[1];
					if (UNLIKELY(header_info & UVC_STREAM_ERR)) {
						strmh->bfh_err |= UVC_STREAM_ERR;
						MARK("bad packet");

						uvc_vc_get_error_code(strmh->devh, &vc_error_code, UVC_GET_CUR);
						uvc_vs_get_error_code(strmh->devh, &vs_error_code, UVC_GET_CUR);
						continue;
					}
#ifdef USE_EOF
					if ((strmh->fid != (header_info & UVC_STREAM_FID)) && strmh->got_bytes) {	
					
						_uvc_swap_buffers(strmh);
					}
					strmh->fid = header_info & UVC_STREAM_FID;
#else
					if (strmh->fid != (header_info & UVC_STREAM_FID)) {	
						_uvc_swap_buffers(strmh);
						strmh->fid = header_info & UVC_STREAM_FID;
					}
#endif
					if (header_info & UVC_STREAM_PTS) {
						
						if (LIKELY(header_len >= 6)) {
							strmh->pts = DW_TO_INT(pktbuf + 2);
						} else {
							MARK("bogus packet: header info has UVC_STREAM_PTS, but no data");
							strmh->pts = 0;
						}
					}

					if (header_info & UVC_STREAM_SCR) {
						
						if (LIKELY(header_len >= 10)) {
							strmh->last_scr = DW_TO_INT(pktbuf + 6);
						} else {
							MARK("bogus packet: header info has UVC_STREAM_SCR, but no data");
							strmh->last_scr = 0;
						}
					}

#ifdef __ANDROID__	
					if (UNLIKELY(strmh->devh->is_isight))
						continue; 
#else
					if (strmh->devh->is_isight) {
						MARK("is_isight");
						continue; 
					}
#endif
				} 

				if (UNLIKELY(pkt->actual_length < header_len)) {
					
					strmh->bfh_err |= UVC_STREAM_ERR;
					MARK("bogus packet: actual_len=%d, header_len=%zd", pkt->actual_length, header_len);
					continue;
				}

				
				
				
				
				if (LIKELY(pkt->actual_length > header_len)) {
					const size_t odd_bytes = pkt->actual_length - header_len;
					assert(strmh->got_bytes + odd_bytes < strmh->size_buf);
					assert(strmh->outbuf);
					assert(pktbuf);
					memcpy(strmh->outbuf + strmh->got_bytes, pktbuf + header_len, odd_bytes);
					strmh->got_bytes += odd_bytes;
				}
#ifdef USE_EOF
				if ((pktbuf[1] & STREAM_HEADER_BFH_EOF) && strmh->got_bytes != 0) {
					
					_uvc_swap_buffers(strmh);
				}
#endif
			} else {	
				strmh->bfh_err |= UVC_STREAM_ERR;
				MARK("libusb_get_iso_packet_buffer_simple returned null");
				continue;
			}
		}	
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		strmh->running = 0;	
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_ERROR:
		UVC_DEBUG("not retrying transfer, status = %d", transfer->status);

		_uvc_delete_transfer(transfer);
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_OVERFLOW:
		UVC_DEBUG("retrying transfer, status = %d", transfer->status);

		break;
	}

	if (LIKELY(strmh->running)) {
		libusb_submit_transfer(transfer);
	} else {
		
		
		_uvc_delete_transfer(transfer);
	}
}
#endif


uvc_error_t uvc_start_streaming(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr,
		uint8_t flags) {
	return uvc_start_streaming_bandwidth(devh, ctrl, cb, user_ptr, 0, flags);
}


uvc_error_t uvc_start_streaming_bandwidth(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr,
		float bandwidth_factor,
		uint8_t flags) {
	uvc_error_t ret;
	uvc_stream_handle_t *strmh;

	ret = uvc_stream_open_ctrl(devh, &strmh, ctrl);
	if (UNLIKELY(ret != UVC_SUCCESS))
		return ret;

	ret = uvc_stream_start_bandwidth(strmh, cb, user_ptr, bandwidth_factor, flags);
	if (UNLIKELY(ret != UVC_SUCCESS)) {
		uvc_stream_close(strmh);
		return ret;
	}

	return UVC_SUCCESS;
}


uvc_error_t uvc_start_iso_streaming(uvc_device_handle_t *devh,
		uvc_stream_ctrl_t *ctrl, uvc_frame_callback_t *cb, void *user_ptr) {
	return uvc_start_streaming_bandwidth(devh, ctrl, cb, user_ptr, 0.0f, 0);
}

static uvc_stream_handle_t *_uvc_get_stream_by_interface(
		uvc_device_handle_t *devh, int interface_idx) {
	uvc_stream_handle_t *strmh;

	DL_FOREACH(devh->streams, strmh)
	{
		if (strmh->stream_if->bInterfaceNumber == interface_idx)
			return strmh;
	}

	return NULL;
}

static uvc_streaming_interface_t *_uvc_get_stream_if(uvc_device_handle_t *devh,
		int interface_idx) {
	uvc_streaming_interface_t *stream_if;

	DL_FOREACH(devh->info->stream_ifs, stream_if)
	{
		if (stream_if->bInterfaceNumber == interface_idx)
			return stream_if;
	}

	return NULL;
}


uvc_error_t uvc_stream_open_ctrl(uvc_device_handle_t *devh,
		uvc_stream_handle_t **strmhp, uvc_stream_ctrl_t *ctrl) {
	
	uvc_stream_handle_t *strmh = NULL;
	uvc_streaming_interface_t *stream_if;
	uvc_error_t ret;

	UVC_ENTER();

	if (UNLIKELY(_uvc_get_stream_by_interface(devh, ctrl->bInterfaceNumber) != NULL)) {
		ret = UVC_ERROR_BUSY; 
		goto fail;
	}

	stream_if = _uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
	if (UNLIKELY(!stream_if)) {
		ret = UVC_ERROR_INVALID_PARAM;
		goto fail;
	}

	strmh = calloc(1, sizeof(*strmh));
	if (UNLIKELY(!strmh)) {
		ret = UVC_ERROR_NO_MEM;
		goto fail;
	}
	strmh->devh = devh;
	strmh->stream_if = stream_if;
	strmh->frame.library_owns_data = 1;

	ret = uvc_claim_if(strmh->devh, strmh->stream_if->bInterfaceNumber);
	if (UNLIKELY(ret != UVC_SUCCESS))
		goto fail;

	ret = uvc_stream_ctrl(strmh, ctrl);
	if (UNLIKELY(ret != UVC_SUCCESS))
		goto fail;

	
	strmh->running = 0;
	
	strmh->outbuf = malloc(LIBUVC_XFER_BUF_SIZE);
	strmh->holdbuf = malloc(LIBUVC_XFER_BUF_SIZE);
	strmh->size_buf = LIBUVC_XFER_BUF_SIZE;	

	pthread_mutex_init(&strmh->cb_mutex, NULL);
	pthread_cond_init(&strmh->cb_cond, NULL);

	DL_APPEND(devh->streams, strmh);

	*strmhp = strmh;

	UVC_EXIT(0);
	return UVC_SUCCESS;

fail:
	if (strmh)
		free(strmh);
	UVC_EXIT(ret);
	return ret;
}


uvc_error_t uvc_stream_start(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags) {
	return uvc_stream_start_bandwidth(strmh, cb, user_ptr, 0, flags);
}


uvc_error_t uvc_stream_start_bandwidth(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr, float bandwidth_factor, uint8_t flags) {
	
	const struct libusb_interface *interface;
	int interface_id;
	char isochronous;
	uvc_frame_desc_t *frame_desc;
	uvc_format_desc_t *format_desc;
	uvc_stream_ctrl_t *ctrl;
	uvc_error_t ret;
	
	size_t total_transfer_size;
	struct libusb_transfer *transfer;
	int transfer_id;

	ctrl = &strmh->cur_ctrl;

	UVC_ENTER();

	if (UNLIKELY(strmh->running)) {
		UVC_EXIT(UVC_ERROR_BUSY);
		return UVC_ERROR_BUSY;
	}

	strmh->running = 1;
	strmh->seq = 0;
	strmh->fid = 0;
	strmh->pts = 0;
	strmh->last_scr = 0;
	strmh->bfh_err = 0;	

	frame_desc = uvc_find_frame_desc_stream(strmh, ctrl->bFormatIndex, ctrl->bFrameIndex);
	if (UNLIKELY(!frame_desc)) {
		ret = UVC_ERROR_INVALID_PARAM;
		LOGE("UVC_ERROR_INVALID_PARAM");
		goto fail;
	}
	format_desc = frame_desc->parent;

	strmh->frame_format = uvc_frame_format_for_guid(format_desc->guidFormat);
	if (UNLIKELY(strmh->frame_format == UVC_FRAME_FORMAT_UNKNOWN)) {
		ret = UVC_ERROR_NOT_SUPPORTED;
		LOGE("unlnown frame format");
		goto fail;
	}
	const uint32_t dwMaxVideoFrameSize = ctrl->dwMaxVideoFrameSize <= frame_desc->dwMaxVideoFrameBufferSize
		? ctrl->dwMaxVideoFrameSize : frame_desc->dwMaxVideoFrameBufferSize;

	
	interface_id = strmh->stream_if->bInterfaceNumber;
	interface = &strmh->devh->info->config->interface[interface_id];

	
	isochronous = interface->num_altsetting > 1;

	if (isochronous) {
		MARK("isochronous transfer mode:num_altsetting=%d", interface->num_altsetting);
		
		const struct libusb_interface_descriptor *altsetting;
		const struct libusb_endpoint_descriptor *endpoint;
		
		size_t config_bytes_per_packet;
		
		size_t packets_per_transfer;
		
		size_t total_transfer_size;
		
		size_t endpoint_bytes_per_packet;
		
		int alt_idx, ep_idx;

		struct libusb_transfer *transfer;
		int transfer_id;
		
		if ((bandwidth_factor > 0) && (bandwidth_factor < 1.0f)) {
			config_bytes_per_packet = (size_t)(strmh->cur_ctrl.dwMaxPayloadTransferSize * bandwidth_factor);
			if (!config_bytes_per_packet) {
				config_bytes_per_packet = strmh->cur_ctrl.dwMaxPayloadTransferSize;
			}
		} else {
			config_bytes_per_packet = strmh->cur_ctrl.dwMaxPayloadTransferSize;
		}





		if (UNLIKELY(!config_bytes_per_packet)) {	
			ret = UVC_ERROR_IO;
			LOGE("config_bytes_per_packet is zero");
			goto fail;
		}

		
		const int num_alt = interface->num_altsetting - 1;
		for (alt_idx = 0; alt_idx <= num_alt ; alt_idx++) {
			altsetting = interface->altsetting + alt_idx;
			endpoint_bytes_per_packet = 0;

			
			for (ep_idx = 0; ep_idx < altsetting->bNumEndpoints; ep_idx++) {
				endpoint = altsetting->endpoint + ep_idx;
				if (endpoint->bEndpointAddress == format_desc->parent->bEndpointAddress) {
					endpoint_bytes_per_packet = endpoint->wMaxPacketSize;
					
					
					
					
					
					
					
					endpoint_bytes_per_packet
						= (endpoint_bytes_per_packet & 0x07ff)
							* (((endpoint_bytes_per_packet >> 11) & 3) + 1);
					break;
				}
			}
			
			if (LIKELY(endpoint_bytes_per_packet)) {
				if ( (endpoint_bytes_per_packet >= config_bytes_per_packet)
					|| (alt_idx == num_alt) ) {	
					
					packets_per_transfer = (dwMaxVideoFrameSize
							+ endpoint_bytes_per_packet - 1)
							/ endpoint_bytes_per_packet;		

					
					if (packets_per_transfer > 32)
						packets_per_transfer = 32;

					total_transfer_size = packets_per_transfer * endpoint_bytes_per_packet;
					break;
				}
			}
		}
		if (UNLIKELY(!endpoint_bytes_per_packet)) {
			LOGE("endpoint_bytes_per_packet is zero");
			ret = UVC_ERROR_INVALID_MODE;
			goto fail;
		}
		if (UNLIKELY(!total_transfer_size)) {
			LOGE("total_transfer_size is zero");
			ret = UVC_ERROR_INVALID_MODE;
			goto fail;
		}

		


		
		MARK("Select the altsetting");
		ret = libusb_set_interface_alt_setting(strmh->devh->usb_devh,
				altsetting->bInterfaceNumber, altsetting->bAlternateSetting);
		if (UNLIKELY(ret != UVC_SUCCESS)) {
			UVC_DEBUG("libusb_set_interface_alt_setting failed");
			goto fail;
		}

		
		MARK("Set up the transfers");
		for (transfer_id = 0; transfer_id < LIBUVC_NUM_TRANSFER_BUFS; ++transfer_id) {
			transfer = libusb_alloc_transfer(packets_per_transfer);
			strmh->transfers[transfer_id] = transfer;
			strmh->transfer_bufs[transfer_id] = malloc(total_transfer_size);

			libusb_fill_iso_transfer(transfer, strmh->devh->usb_devh,
				format_desc->parent->bEndpointAddress,
				strmh->transfer_bufs[transfer_id], total_transfer_size,
				packets_per_transfer, _uvc_stream_callback,
				(void*) strmh, 5000);

			libusb_set_iso_packet_lengths(transfer, endpoint_bytes_per_packet);
		}
	} else {
		MARK("bulk transfer mode");
		
		for (transfer_id = 0; transfer_id < LIBUVC_NUM_TRANSFER_BUFS; ++transfer_id) {
			transfer = libusb_alloc_transfer(0);
			strmh->transfers[transfer_id] = transfer;
			strmh->transfer_bufs[transfer_id] = malloc(strmh->cur_ctrl.dwMaxPayloadTransferSize);
			libusb_fill_bulk_transfer(transfer, strmh->devh->usb_devh,
				format_desc->parent->bEndpointAddress,
				strmh->transfer_bufs[transfer_id],
				strmh->cur_ctrl.dwMaxPayloadTransferSize, _uvc_stream_callback,
				(void *)strmh, 5000);
		}
	}

	strmh->user_cb = cb;
	strmh->user_ptr = user_ptr;

	
	MARK("create callback thread");
	if LIKELY(cb) {
		pthread_create(&strmh->cb_thread, NULL, _uvc_user_caller, (void*) strmh);
	}
	MARK("submit transfers");
	for (transfer_id = 0; transfer_id < LIBUVC_NUM_TRANSFER_BUFS; transfer_id++) {
		ret = libusb_submit_transfer(strmh->transfers[transfer_id]);
		if (UNLIKELY(ret != UVC_SUCCESS)) {
			UVC_DEBUG("libusb_submit_transfer failed");
			break;
		}
	}

	if (UNLIKELY(ret != UVC_SUCCESS)) {
		
		goto fail;
	}

	UVC_EXIT(ret);
	return ret;
fail:
	LOGE("fail");
	strmh->running = 0;
	UVC_EXIT(ret);
	return ret;
}


uvc_error_t uvc_stream_start_iso(uvc_stream_handle_t *strmh,
		uvc_frame_callback_t *cb, void *user_ptr) {
	return uvc_stream_start(strmh, cb, user_ptr, 0);
}


static void *_uvc_user_caller(void *arg) {
	uvc_stream_handle_t *strmh = (uvc_stream_handle_t *) arg;

	uint32_t last_seq = 0;

	for (; 1 ;) {
		pthread_mutex_lock(&strmh->cb_mutex);
		{
			for (; strmh->running && (last_seq == strmh->hold_seq) ;) {
				pthread_cond_wait(&strmh->cb_cond, &strmh->cb_mutex);
			}

			if (UNLIKELY(!strmh->running)) {
				pthread_mutex_unlock(&strmh->cb_mutex);
				break;
			}

			last_seq = strmh->hold_seq;
			if (LIKELY(!strmh->hold_bfh_err))	
				_uvc_populate_frame(strmh);
		}
		pthread_mutex_unlock(&strmh->cb_mutex);

		if (LIKELY(!strmh->hold_bfh_err))	
			strmh->user_cb(&strmh->frame, strmh->user_ptr);	
	}

	return NULL; 
}


void _uvc_populate_frame(uvc_stream_handle_t *strmh) {
	size_t alloc_size = strmh->cur_ctrl.dwMaxVideoFrameSize;
	uvc_frame_t *frame = &strmh->frame;
	uvc_frame_desc_t *frame_desc;

	

	frame_desc = uvc_find_frame_desc(strmh->devh, strmh->cur_ctrl.bFormatIndex,
			strmh->cur_ctrl.bFrameIndex);

	frame->frame_format = strmh->frame_format;

	frame->width = frame_desc->wWidth;
	frame->height = frame_desc->wHeight;
	
	frame->actual_bytes = LIKELY(!strmh->hold_bfh_err) ? strmh->hold_bytes : 0;

	switch (frame->frame_format) {
	case UVC_FRAME_FORMAT_YUYV:
		frame->step = frame->width * 2;
		break;
	case UVC_FRAME_FORMAT_MJPEG:
		frame->step = 0;
		break;
	default:
		frame->step = 0;
		break;
	}

	
	if (UNLIKELY(frame->data_bytes < strmh->hold_bytes)) {
		frame->data = realloc(frame->data, strmh->hold_bytes);	
		frame->data_bytes = strmh->hold_bytes;
	}
	memcpy(frame->data, strmh->holdbuf, strmh->hold_bytes);	

	
}


uvc_error_t uvc_stream_get_frame(uvc_stream_handle_t *strmh,
		uvc_frame_t **frame, int32_t timeout_us) {
	time_t add_secs;
	time_t add_nsecs;
	struct timespec ts;
	struct timeval tv;

	if (UNLIKELY(!strmh->running))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(strmh->user_cb))
		return UVC_ERROR_CALLBACK_EXISTS;

	pthread_mutex_lock(&strmh->cb_mutex);
	{
		if (strmh->last_polled_seq < strmh->hold_seq) {
			_uvc_populate_frame(strmh);
			*frame = &strmh->frame;
			strmh->last_polled_seq = strmh->hold_seq;
		} else if (timeout_us != -1) {
			if (!timeout_us) {
				pthread_cond_wait(&strmh->cb_cond, &strmh->cb_mutex);
			} else {
				add_secs = timeout_us / 1000000;
				add_nsecs = (timeout_us % 1000000) * 1000;
				ts.tv_sec = 0;
				ts.tv_nsec = 0;

#if _POSIX_TIMERS > 0
				clock_gettime(CLOCK_REALTIME, &ts);
#else
				gettimeofday(&tv, NULL);
				ts.tv_sec = tv.tv_sec;
				ts.tv_nsec = tv.tv_usec * 1000;
#endif

				ts.tv_sec += add_secs;
				ts.tv_nsec += add_nsecs;

				pthread_cond_timedwait(&strmh->cb_cond, &strmh->cb_mutex, &ts);
			}

			if (LIKELY(strmh->last_polled_seq < strmh->hold_seq)) {
				_uvc_populate_frame(strmh);
				*frame = &strmh->frame;
				strmh->last_polled_seq = strmh->hold_seq;
			} else {
				*frame = NULL;
			}
		} else {
			*frame = NULL;
		}
	}
	pthread_mutex_unlock(&strmh->cb_mutex);

	return UVC_SUCCESS;
}


void uvc_stop_streaming(uvc_device_handle_t *devh) {
	uvc_stream_handle_t *strmh, *strmh_tmp;

	UVC_ENTER();
	DL_FOREACH_SAFE(devh->streams, strmh, strmh_tmp)
	{
		uvc_stream_close(strmh);
	}
	UVC_EXIT_VOID();
}


uvc_error_t uvc_stream_stop(uvc_stream_handle_t *strmh) {

	int i;
	ENTER();

	if (!strmh) RETURN(UVC_SUCCESS, uvc_error_t);

	if (UNLIKELY(!strmh->running)) {
		UVC_EXIT(UVC_ERROR_INVALID_PARAM);
		RETURN(UVC_ERROR_INVALID_PARAM, uvc_error_t);
	}

	strmh->running = 0;

	pthread_mutex_lock(&strmh->cb_mutex);
	{
		for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++) {
			if (strmh->transfers[i]) {
				int res = libusb_cancel_transfer(strmh->transfers[i]);
				if ((res < 0) && (res != LIBUSB_ERROR_NOT_FOUND)) {
					UVC_DEBUG("libusb_cancel_transfer failed");
					
					
					
					

				}
				if (res == LIBUSB_ERROR_NOT_FOUND && strmh->transfers[i] != NULL) {
                    free(strmh->transfers[i]->buffer);
                    
                    strmh->transfers[i] = NULL;
                }
			}
		}

		
		for (; 1 ;) {
			for (i = 0; i < LIBUVC_NUM_TRANSFER_BUFS; i++) {
				if (strmh->transfers[i] != NULL)
					break;
			}
			if (i == LIBUVC_NUM_TRANSFER_BUFS)
				break;

             ts.tv_sec = 0;
             ts.tv_nsec = 0;

#if _POSIX_TIMERS > 0
             clock_gettime(CLOCK_REALTIME, &ts);
#else
             gettimeofday(&tv, NULL);
             ts.tv_sec = tv.tv_sec;
             ts.tv_nsec = tv.tv_usec * 1000;
#endif
             ts.tv_sec += 1;
             ts.tv_nsec += 0;
			if (pthread_cond_timedwait(&strmh->cb_cond, &strmh->cb_mutex, &ts) == ETIMEDOUT) {
                break;
			}
		}
		
		pthread_cond_broadcast(&strmh->cb_cond);
	}
	pthread_mutex_unlock(&strmh->cb_mutex);

	

	if (strmh->user_cb) {
		
		pthread_join(strmh->cb_thread, NULL);
	}

	RETURN(UVC_SUCCESS, uvc_error_t);
}


void uvc_stream_close(uvc_stream_handle_t *strmh) {
	UVC_ENTER();

	if (!strmh) { UVC_EXIT_VOID() };

	if (strmh->running)
		uvc_stream_stop(strmh);

	uvc_release_if(strmh->devh, strmh->stream_if->bInterfaceNumber);

	if (strmh->frame.data) {
		free(strmh->frame.data);
		strmh->frame.data = NULL;
	}

	if (strmh->outbuf) {
		free(strmh->outbuf);
		strmh->outbuf = NULL;
	}
	if (strmh->holdbuf) {
		free(strmh->holdbuf);
		strmh->holdbuf = NULL;
	}

	pthread_cond_destroy(&strmh->cb_cond);
	pthread_mutex_destroy(&strmh->cb_mutex);

	DL_DELETE(strmh->devh->streams, strmh);
	free(strmh);

	UVC_EXIT_VOID();
}
