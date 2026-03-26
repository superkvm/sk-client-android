




#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_MPGLIB
#define hip_global_struct mpstr_tag 
#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "interface.h"

#include "util.h"



#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED

int CDECL lame_decode_init(void);
int CDECL lame_decode(
        unsigned char *  mp3buf,
        int              len,
        short            pcm_l[],
        short            pcm_r[] );
int CDECL lame_decode_headers(
        unsigned char*   mp3buf,
        int              len,
        short            pcm_l[],
        short            pcm_r[],
        mp3data_struct*  mp3data );
int CDECL lame_decode1(
        unsigned char*  mp3buf,
        int             len,
        short           pcm_l[],
        short           pcm_r[] );
int CDECL lame_decode1_headers(
        unsigned char*   mp3buf,
        int              len,
        short            pcm_l[],
        short            pcm_r[],
        mp3data_struct*  mp3data );
int CDECL lame_decode1_headersB(
        unsigned char*   mp3buf,
        int              len,
        short            pcm_l[],
        short            pcm_r[],
        mp3data_struct*  mp3data,
        int              *enc_delay,
        int              *enc_padding );
int CDECL lame_decode_exit(void);
#endif


static MPSTR   mp;

int
lame_decode_exit(void)
{
    ExitMP3(&mp);
    return 0;
}


int
lame_decode_init(void)
{
    (void) InitMP3(&mp);
    return 0;
}





#define COPY_MONO(DST_TYPE, SRC_TYPE)                                                           \
    DST_TYPE *pcm_l = (DST_TYPE *)pcm_l_raw;                                                    \
    SRC_TYPE const *p_samples = (SRC_TYPE const *)p;                                            \
    for (i = 0; i < processed_samples; i++)                                                     \
      *pcm_l++ = (DST_TYPE)(*p_samples++);


#define COPY_STEREO(DST_TYPE, SRC_TYPE)                                                         \
    DST_TYPE *pcm_l = (DST_TYPE *)pcm_l_raw, *pcm_r = (DST_TYPE *)pcm_r_raw;                    \
    SRC_TYPE const *p_samples = (SRC_TYPE const *)p;                                            \
    for (i = 0; i < processed_samples; i++) {                                                   \
      *pcm_l++ = (DST_TYPE)(*p_samples++);                                                      \
      *pcm_r++ = (DST_TYPE)(*p_samples++);                                                      \
    }





static int
decode1_headersB_clipchoice(PMPSTR pmp, unsigned char *buffer, int len,
                            char pcm_l_raw[], char pcm_r_raw[], mp3data_struct * mp3data,
                            int *enc_delay, int *enc_padding,
                            char *p, size_t psize, int decoded_sample_size,
                            int (*decodeMP3_ptr) (PMPSTR, unsigned char *, int, char *, int,
                            int *))
{
    static const int smpls[2][4] = {
        
        {0, 384, 1152, 1152}, 
        {0, 384, 1152, 576} 
    };

    int     processed_bytes;
    int     processed_samples; 
    int     ret;
    int     i;

    mp3data->header_parsed = 0;

    ret = (*decodeMP3_ptr) (pmp, buffer, len, p, (int) psize, &processed_bytes);
    
    if (pmp->header_parsed || pmp->fsizeold > 0 || pmp->framesize > 0) {
        mp3data->header_parsed = 1;
        mp3data->stereo = pmp->fr.stereo;
        mp3data->samplerate = freqs[pmp->fr.sampling_frequency];
        mp3data->mode = pmp->fr.mode;
        mp3data->mode_ext = pmp->fr.mode_ext;
        mp3data->framesize = smpls[pmp->fr.lsf][pmp->fr.lay];

        
        if (pmp->fsizeold > 0) 
            mp3data->bitrate = 8 * (4 + pmp->fsizeold) * mp3data->samplerate /
                (1.e3 * mp3data->framesize) + 0.5;
        else if (pmp->framesize > 0)
            mp3data->bitrate = 8 * (4 + pmp->framesize) * mp3data->samplerate /
                (1.e3 * mp3data->framesize) + 0.5;
        else
            mp3data->bitrate = tabsel_123[pmp->fr.lsf][pmp->fr.lay - 1][pmp->fr.bitrate_index];



        if (pmp->num_frames > 0) {
            
            mp3data->totalframes = pmp->num_frames;
            mp3data->nsamp = mp3data->framesize * pmp->num_frames;
            *enc_delay = pmp->enc_delay;
            *enc_padding = pmp->enc_padding;
        }
    }

    switch (ret) {
    case MP3_OK:
        switch (pmp->fr.stereo) {
        case 1:
            processed_samples = processed_bytes / decoded_sample_size;
            if (decoded_sample_size == sizeof(short)) {
                COPY_MONO(short, short)
            }
            else {
                COPY_MONO(sample_t, FLOAT)
            }
            break;
        case 2:
            processed_samples = (processed_bytes / decoded_sample_size) >> 1;
            if (decoded_sample_size == sizeof(short)) {
                COPY_STEREO(short, short)
            }
            else {
                COPY_STEREO(sample_t, FLOAT)
            }
            break;
        default:
            processed_samples = -1;
            assert(0);
            break;
        }
        break;

    case MP3_NEED_MORE:
        processed_samples = 0;
        break;

    case MP3_ERR:
        processed_samples = -1;
        break;

    default:
        processed_samples = -1;
        assert(0);
        break;
    }

    
    
    return processed_samples;
}


#define OUTSIZE_CLIPPED   (4096*sizeof(short))

int
lame_decode1_headersB(unsigned char *buffer,
                      int len,
                      short pcm_l[], short pcm_r[], mp3data_struct * mp3data,
                      int *enc_delay, int *enc_padding)
{
    static char out[OUTSIZE_CLIPPED];

    return decode1_headersB_clipchoice(&mp, buffer, len, (char *) pcm_l, (char *) pcm_r, mp3data,
                                       enc_delay, enc_padding, out, OUTSIZE_CLIPPED,
                                       sizeof(short), decodeMP3);
}







int
lame_decode1_headers(unsigned char *buffer,
                     int len, short pcm_l[], short pcm_r[], mp3data_struct * mp3data)
{
    int     enc_delay, enc_padding;
    return lame_decode1_headersB(buffer, len, pcm_l, pcm_r, mp3data, &enc_delay, &enc_padding);
}


int
lame_decode1(unsigned char *buffer, int len, short pcm_l[], short pcm_r[])
{
    mp3data_struct mp3data;

    return lame_decode1_headers(buffer, len, pcm_l, pcm_r, &mp3data);
}




int
lame_decode_headers(unsigned char *buffer,
                    int len, short pcm_l[], short pcm_r[], mp3data_struct * mp3data)
{
    int     ret;
    int     totsize = 0;     

    for (;;) {
        switch (ret = lame_decode1_headers(buffer, len, pcm_l + totsize, pcm_r + totsize, mp3data)) {
        case -1:
            return ret;
        case 0:
            return totsize;
        default:
            totsize += ret;
            len = 0;    
            break;
        }
    }
}


int
lame_decode(unsigned char *buffer, int len, short pcm_l[], short pcm_r[])
{
    mp3data_struct mp3data;

    return lame_decode_headers(buffer, len, pcm_l, pcm_r, &mp3data);
}




hip_t hip_decode_init(void)
{
    hip_t hip = calloc(1, sizeof(hip_global_flags));
    InitMP3(hip);
    return hip;
}


int hip_decode_exit(hip_t hip)
{
    if (hip) {
        ExitMP3(hip);
        free(hip);
    }
    return 0;
}



#define OUTSIZE_UNCLIPPED (1152*2*sizeof(FLOAT))

int
hip_decode1_unclipped(hip_t hip, unsigned char *buffer, size_t len, sample_t pcm_l[], sample_t pcm_r[])
{
    static char out[OUTSIZE_UNCLIPPED];
    mp3data_struct mp3data;
    int     enc_delay, enc_padding;

    if (hip) {
        return decode1_headersB_clipchoice(hip, buffer, len, (char *) pcm_l, (char *) pcm_r, &mp3data,
                                           &enc_delay, &enc_padding, out, OUTSIZE_UNCLIPPED,
                                           sizeof(FLOAT), decodeMP3_unclipped);
    }
    return 0;
}



int
hip_decode1_headers(hip_t hip, unsigned char *buffer,
                     size_t len, short pcm_l[], short pcm_r[], mp3data_struct * mp3data)
{
    int     enc_delay, enc_padding;
    return hip_decode1_headersB(hip, buffer, len, pcm_l, pcm_r, mp3data, &enc_delay, &enc_padding);
}


int
hip_decode1(hip_t hip, unsigned char *buffer, size_t len, short pcm_l[], short pcm_r[])
{
    mp3data_struct mp3data;
    return hip_decode1_headers(hip, buffer, len, pcm_l, pcm_r, &mp3data);
}




int
hip_decode_headers(hip_t hip, unsigned char *buffer,
                    size_t len, short pcm_l[], short pcm_r[], mp3data_struct * mp3data)
{
    int     ret;
    int     totsize = 0;     

    for (;;) {
        switch (ret = hip_decode1_headers(hip, buffer, len, pcm_l + totsize, pcm_r + totsize, mp3data)) {
        case -1:
            return ret;
        case 0:
            return totsize;
        default:
            totsize += ret;
            len = 0;    
            break;
        }
    }
}


int
hip_decode(hip_t hip, unsigned char *buffer, size_t len, short pcm_l[], short pcm_r[])
{
    mp3data_struct mp3data;
    return hip_decode_headers(hip, buffer, len, pcm_l, pcm_r, &mp3data);
}


int
hip_decode1_headersB(hip_t hip, unsigned char *buffer,
                      size_t len,
                      short pcm_l[], short pcm_r[], mp3data_struct * mp3data,
                      int *enc_delay, int *enc_padding)
{
    static char out[OUTSIZE_CLIPPED];
    if (hip) {
        return decode1_headersB_clipchoice(hip, buffer, len, (char *) pcm_l, (char *) pcm_r, mp3data,
                                           enc_delay, enc_padding, out, OUTSIZE_CLIPPED,
                                           sizeof(short), decodeMP3);
    }
    return -1;
}


void hip_set_pinfo(hip_t hip, plotting_data* pinfo)
{
    if (hip) {
        hip->pinfo = pinfo;
    }
}



void hip_set_errorf(hip_t hip, lame_report_function func)
{
    if (hip) {
        hip->report_err = func;
    }
}

void hip_set_debugf(hip_t hip, lame_report_function func)
{
    if (hip) {
        hip->report_dbg = func;
    }
}

void hip_set_msgf  (hip_t hip, lame_report_function func)
{
    if (hip) {
        hip->report_msg = func;
    }
}

#endif


