

#ifndef LAME_UTIL_H
#define LAME_UTIL_H

#include "l3side.h"
#include "id3tag.h"
#include "lame_global_flags.h"

#ifdef __cplusplus
extern  "C" {
#endif



#ifndef FALSE
#define         FALSE                   0
#endif

#ifndef TRUE
#define         TRUE                    (!FALSE)
#endif

#ifdef UINT_MAX
# define         MAX_U_32_NUM            UINT_MAX
#else
# define         MAX_U_32_NUM            0xFFFFFFFF
#endif

#ifndef PI
# ifdef M_PI
#  define       PI                      M_PI
# else
#  define       PI                      3.14159265358979323846
# endif
#endif


#ifdef M_LN2
# define        LOG2                    M_LN2
#else
# define        LOG2                    0.69314718055994530942
#endif

#ifdef M_LN10
# define        LOG10                   M_LN10
#else
# define        LOG10                   2.30258509299404568402
#endif


#ifdef M_SQRT2
# define        SQRT2                   M_SQRT2
#else
# define        SQRT2                   1.41421356237309504880
#endif


#define         CRC16_POLYNOMIAL        0x8005

#define MAX_BITS_PER_CHANNEL 4095
#define MAX_BITS_PER_GRANULE 7680


#define         BUFFER_SIZE     LAME_MAXMP3BUFFER

#define         Min(A, B)       ((A) < (B) ? (A) : (B))
#define         Max(A, B)       ((A) > (B) ? (A) : (B))


#ifdef USE_FAST_LOG
#define         FAST_LOG10(x)       (fast_log2(x)*(LOG2/LOG10))
#define         FAST_LOG(x)         (fast_log2(x)*LOG2)
#define         FAST_LOG10_X(x,y)   (fast_log2(x)*(LOG2/LOG10*(y)))
#define         FAST_LOG_X(x,y)     (fast_log2(x)*(LOG2*(y)))
#else
#define         FAST_LOG10(x)       log10(x)
#define         FAST_LOG(x)         log(x)
#define         FAST_LOG10_X(x,y)   (log10(x)*(y))
#define         FAST_LOG_X(x,y)     (log(x)*(y))
#endif


    struct replaygain_data;
#ifndef replaygain_data_defined
#define replaygain_data_defined
    typedef struct replaygain_data replaygain_t;
#endif
    struct plotting_data;
#ifndef plotting_data_defined
#define plotting_data_defined
    typedef struct plotting_data plotting_data;
#endif



    typedef struct {
        void   *aligned;     
        void   *pointer;     
    } aligned_pointer_t;

    void    malloc_aligned(aligned_pointer_t * ptr, unsigned int size, unsigned int bytes);
    void    free_aligned(aligned_pointer_t * ptr);


    typedef void (*iteration_loop_t) (lame_internal_flags * gfc, const FLOAT pe[2][2],
                                      const FLOAT ms_ratio[2], const III_psy_ratio ratio[2][2]);


    

    typedef struct bit_stream_struc {
        unsigned char *buf;  
        int     buf_size;    
        int     totbit;      
        int     buf_byte_idx; 
        int     buf_bit_idx; 

        
    } Bit_stream_struc;



    typedef struct {
        int     sum;         
        int     seen;        
        int     want;        
        int     pos;         
        int     size;        
        int    *bag;         
        unsigned int nVbrNumFrames;
        unsigned long nBytesWritten;
        
        unsigned int TotalFrameSize;
    } VBR_seek_info_t;


    
    typedef struct {
        int     use_adjust;  
        FLOAT   aa_sensitivity_p; 
        FLOAT   adjust_factor; 
        FLOAT   adjust_limit; 
        FLOAT   decay;       
        FLOAT   floor;       
        FLOAT   l[SBMAX_l];  
        FLOAT   s[SBMAX_s];  
        FLOAT   psfb21[PSFB21]; 
        FLOAT   psfb12[PSFB12]; 
        FLOAT   cb_l[CBANDS]; 
        FLOAT   cb_s[CBANDS]; 
        FLOAT   eql_w[BLKSIZE / 2]; 
    } ATH_t;

    

    typedef struct {
        FLOAT   masking_lower[CBANDS];
        FLOAT   minval[CBANDS];
        FLOAT   rnumlines[CBANDS];
        FLOAT   mld_cb[CBANDS];
        FLOAT   mld[Max(SBMAX_l,SBMAX_s)];
        FLOAT   bo_weight[Max(SBMAX_l,SBMAX_s)]; 
        FLOAT   attack_threshold; 
        int     s3ind[CBANDS][2];
        int     numlines[CBANDS];
        int     bm[Max(SBMAX_l,SBMAX_s)];
        int     bo[Max(SBMAX_l,SBMAX_s)];
        int     npart;
        int     n_sb; 
        FLOAT  *s3;
    } PsyConst_CB2SB_t;


    
    typedef struct {
        PsyConst_CB2SB_t l;
        PsyConst_CB2SB_t s;
        PsyConst_CB2SB_t l_to_s;
        FLOAT   attack_threshold[4];
        FLOAT   decay;
        int     force_short_block_calc;
    } PsyConst_t;


    typedef struct {

        FLOAT   nb_l1[4][CBANDS], nb_l2[4][CBANDS];
        FLOAT   nb_s1[4][CBANDS], nb_s2[4][CBANDS];

        III_psy_xmin thm[4];
        III_psy_xmin en[4];

        
        FLOAT   loudness_sq_save[2]; 

        FLOAT   tot_ener[4];

        FLOAT   last_en_subshort[4][9];
        int     last_attacks[4];

        int     blocktype_old[2];
    } PsyStateVar_t;


    typedef struct {
        
        FLOAT   loudness_sq[2][2]; 
    } PsyResult_t;


    
    typedef struct {
        
        FLOAT   sb_sample[2][2][18][SBLIMIT];
        FLOAT   amp_filter[32];

        
        
#define BPC 320
        double  itime[2]; 
        sample_t *inbuf_old[2];
        sample_t *blackfilt[2 * BPC + 1];

        FLOAT   pefirbuf[19];
        
        
        int     frac_SpF;
        int     slot_lag;

        
        
        
#define MAX_HEADER_BUF 256
#define MAX_HEADER_LEN 40    
        struct {
            int     write_timing;
            int     ptr;
            char    buf[MAX_HEADER_LEN];
        } header[MAX_HEADER_BUF];

        int     h_ptr;
        int     w_ptr;
        int     ancillary_flag;

        
        int     ResvSize;    
        int     ResvMax;     

        int     in_buffer_nsamples;
        sample_t *in_buffer_0;
        sample_t *in_buffer_1;

#ifndef  MFSIZE
# define MFSIZE  ( 3*1152 + ENCDELAY - MDCTDELAY )
#endif
        sample_t mfbuf[2][MFSIZE];

        int     mf_samples_to_encode;
        int     mf_size;

    } EncStateVar_t;


    typedef struct {
        
        int     bitrate_channelmode_hist[16][4 + 1];
        int     bitrate_blocktype_hist[16][4 + 1 + 1]; 

        int     bitrate_index;
        int     frame_number; 
        int     padding;     
        int     mode_ext;
        int     encoder_delay;
        int     encoder_padding; 
    } EncResult_t;


    
    typedef struct {
        
        FLOAT   longfact[SBMAX_l];
        FLOAT   shortfact[SBMAX_s];
        FLOAT   masking_lower;
        FLOAT   mask_adjust; 
        FLOAT   mask_adjust_short; 
        int     OldValue[2];
        int     CurrentStep[2];
        int     pseudohalf[SFBMAX];
        int     sfb21_extra; 
        int     substep_shaping; 


        char    bv_scf[576];
    } QntStateVar_t;


    typedef struct {
        replaygain_t *rgdata;
        
    } RpgStateVar_t;


    typedef struct {
        FLOAT   noclipScale; 
        sample_t PeakSample;
        int     RadioGain;
        int     noclipGainChange; 
    } RpgResult_t;


    typedef struct {
        int     version;     
        int     samplerate_index;
        int     sideinfo_len;

        int     noise_shaping; 

        int     subblock_gain; 
        int     use_best_huffman; 
        int     noise_shaping_amp; 

        int     noise_shaping_stop; 


        int     full_outer_loop; 

        int     lowpassfreq;
        int     highpassfreq;
        int     samplerate_in; 
        int     samplerate_out; 
        int     channels_in; 
        int     channels_out; 
        int     mode_gr;     
        int     force_ms;    

        int     quant_comp;
        int     quant_comp_short;

        int     use_temporal_masking_effect;
        int     use_safe_joint_stereo;

        int     preset;

        vbr_mode vbr;
        int     vbr_avg_bitrate_kbps;
        int     vbr_min_bitrate_index; 
        int     vbr_max_bitrate_index; 
        int     avg_bitrate;
        int     enforce_min_bitrate; 

        int     findReplayGain; 
        int     findPeakSample;
        int     decode_on_the_fly; 
        int     analysis;
        int     disable_reservoir;
        int     buffer_constraint;  
        int     free_format;
        int     write_lame_tag; 

        int     error_protection; 
        int     copyright;   
        int     original;    
        int     extension;   
        int     emphasis;    


        MPEG_mode mode;
        short_block_t short_blocks;

        float   interChRatio;
        float   msfix;       
        float   ATH_offset_db;
        float   ATH_offset_factor;
        float   ATHcurve;    
        int     ATHtype;
        int     ATHonly;     
        int     ATHshort;    
        int     noATH;       
        
        float   ATHfixpoint;

        float   adjust_alto_db;
        float   adjust_bass_db;
        float   adjust_treble_db;
        float   adjust_sfb21_db;

        float   compression_ratio; 

        
        FLOAT   lowpass1, lowpass2; 
        FLOAT   highpass1, highpass2; 

        
        FLOAT   pcm_transform[2][2];

        FLOAT   minval;
    } SessionConfig_t;


    struct lame_internal_flags {

  

        
#  define  LAME_ID   0xFFF88E3B
        unsigned long class_id;

        int     lame_encode_frame_init;
        int     iteration_init_init;
        int     fill_buffer_resample_init;

        SessionConfig_t cfg;

        
        Bit_stream_struc bs;
        III_side_info_t l3_side;

        scalefac_struct scalefac_band;

        PsyStateVar_t sv_psy; 
        PsyResult_t ov_psy;
        EncStateVar_t sv_enc; 
        EncResult_t ov_enc;
        QntStateVar_t sv_qnt; 

        RpgStateVar_t sv_rpg;
        RpgResult_t ov_rpg;

        
        struct id3tag_spec tag_spec;
        uint16_t nMusicCRC;

        uint16_t _unused;

        
        struct {
            unsigned int MMX:1; 
            unsigned int AMD_3DNow:1; 
            unsigned int SSE:1; 
            unsigned int SSE2:1; 
            unsigned int _unused:28;
        } CPU_features;


        VBR_seek_info_t VBR_seek_table; 

        ATH_t  *ATH;         

        PsyConst_t *cd_psy;

        
        plotting_data *pinfo;
        hip_t hip;

        iteration_loop_t iteration_loop;

        
        int     (*choose_table) (const int *ix, const int *const end, int *const s);
        void    (*fft_fht) (FLOAT *, int);
        void    (*init_xrpow_core) (gr_info * const cod_info, FLOAT xrpow[576], int upper,
                                    FLOAT * sum);

        lame_report_function report_msg;
        lame_report_function report_dbg;
        lame_report_function report_err;
    };

#ifndef lame_internal_flags_defined
#define lame_internal_flags_defined
    typedef struct lame_internal_flags lame_internal_flags;
#endif



    void    freegfc(lame_internal_flags * const gfc);
    void    free_id3tag(lame_internal_flags * const gfc);
    extern int BitrateIndex(int, int, int);
    extern int FindNearestBitrate(int, int, int);
    extern int map2MP3Frequency(int freq);
    extern int SmpFrqIndex(int, int *const);
    extern int nearestBitrateFullIndex(uint16_t brate);
    extern FLOAT ATHformula(SessionConfig_t const *cfg, FLOAT freq);
    extern FLOAT freq2bark(FLOAT freq);
    void    disable_FPE(void);


    extern void init_log_table(void);

    extern  float  fast_log2(float x);
    int     isResamplingNecessary(SessionConfig_t const* cfg);

    void    fill_buffer(lame_internal_flags * gfc,
                        sample_t *const mfbuf[2],
                        sample_t const *const in_buffer[2], int nsamples, int *n_in, int *n_out);


    int     hip_decode1_unclipped(hip_t hip, unsigned char *mp3buf,
                                   size_t len, sample_t pcm_l[], sample_t pcm_r[]);


    extern int has_MMX(void);
    extern int has_3DNow(void);
    extern int has_SSE(void);
    extern int has_SSE2(void);





    extern void lame_report_def(const char* format, va_list args);
    extern void lame_report_fnc(lame_report_function print_f, const char *, ...);
    extern void lame_errorf(const lame_internal_flags * gfc, const char *, ...);
    extern void lame_debugf(const lame_internal_flags * gfc, const char *, ...);
    extern void lame_msgf(const lame_internal_flags * gfc, const char *, ...);
#define DEBUGF  lame_debugf
#define ERRORF  lame_errorf
#define MSGF    lame_msgf

    int     is_lame_internal_flags_valid(const lame_internal_flags * gfp);
    
    extern void hip_set_pinfo(hip_t hip, plotting_data* pinfo);

#ifdef __cplusplus
}
#endif
#endif                       
