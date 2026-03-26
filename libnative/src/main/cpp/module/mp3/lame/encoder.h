


#ifndef LAME_ENCODER_H
#define LAME_ENCODER_H







#define ENCDELAY      576





#define POSTDELAY   1152





#define MDCTDELAY     48
#define FFTOFFSET     (224+MDCTDELAY)



#define DECDELAY      528



#define SBLIMIT       32


#define CBANDS        64


#define SBPSY_l       21
#define SBPSY_s       12


#define SBMAX_l       22
#define SBMAX_s       13
#define PSFB21         6
#define PSFB12         6




#define BLKSIZE       1024
#define HBLKSIZE      (BLKSIZE/2 + 1)
#define BLKSIZE_s     256
#define HBLKSIZE_s    (BLKSIZE_s/2 + 1)



#define NORM_TYPE     0
#define START_TYPE    1
#define SHORT_TYPE    2
#define STOP_TYPE     3


#if 0
#define MPG_MD_LR_LR  0
#define MPG_MD_LR_I   1
#define MPG_MD_MS_LR  2
#define MPG_MD_MS_I   3
#endif
enum MPEGChannelMode
{   MPG_MD_LR_LR = 0
,   MPG_MD_LR_I  = 1
,   MPG_MD_MS_LR = 2
,   MPG_MD_MS_I  = 3
};

#ifndef lame_internal_flags_defined
#define lame_internal_flags_defined
struct lame_internal_flags;
typedef struct lame_internal_flags lame_internal_flags;
#endif

int     lame_encode_mp3_frame(lame_internal_flags * gfc,
                              sample_t const *inbuf_l,
                              sample_t const *inbuf_r, unsigned char *mp3buf, int mp3buf_size);

#endif 
