

#ifndef LAME_QUANTIZE_PVT_H
#define LAME_QUANTIZE_PVT_H

#define IXMAX_VAL 8206  




#define PRECALC_SIZE (IXMAX_VAL+2)


extern const int nr_of_sfb_block[6][3][4];
extern const int pretab[SBMAX_l];
extern const int slen1_tab[16];
extern const int slen2_tab[16];

extern const scalefac_struct sfBandIndex[9];

extern FLOAT pow43[PRECALC_SIZE];
#ifdef TAKEHIRO_IEEE754_HACK
extern FLOAT adj43asm[PRECALC_SIZE];
#else
extern FLOAT adj43[PRECALC_SIZE];
#endif

#define Q_MAX (256+1)
#define Q_MAX2 116      

extern FLOAT pow20[Q_MAX + Q_MAX2 + 1];
extern FLOAT ipow20[Q_MAX];

typedef struct calc_noise_result_t {
    FLOAT   over_noise;      
    FLOAT   tot_noise;       
    FLOAT   max_noise;       
    int     over_count;      
    int     over_SSD;        
    int     bits;
} calc_noise_result;



typedef struct calc_noise_data_t {
    int     global_gain;
    int     sfb_count1;
    int     step[39];
    FLOAT   noise[39];
    FLOAT   noise_log[39];
} calc_noise_data;


int     on_pe(lame_internal_flags * gfc, const FLOAT pe[2][2],
              int targ_bits[2], int mean_bits, int gr, int cbr);

void    reduce_side(int targ_bits[2], FLOAT ms_ener_ratio, int mean_bits, int max_bits);


void    iteration_init(lame_internal_flags * gfc);


int     calc_xmin(lame_internal_flags const *gfc,
                  III_psy_ratio const *const ratio, gr_info * const cod_info, FLOAT * l3_xmin);

int     calc_noise(const gr_info * const cod_info,
                   const FLOAT * l3_xmin,
                   FLOAT * distort, calc_noise_result * const res, calc_noise_data * prev_noise);

void    set_frame_pinfo(lame_internal_flags * gfc, const III_psy_ratio ratio[2][2]);






int     count_bits(lame_internal_flags const *const gfc, const FLOAT * const xr,
                   gr_info * const cod_info, calc_noise_data * prev_noise);
int     noquant_count_bits(lame_internal_flags const *const gfc,
                           gr_info * const cod_info, calc_noise_data * prev_noise);


void    best_huffman_divide(const lame_internal_flags * const gfc, gr_info * const cod_info);

void    best_scalefac_store(const lame_internal_flags * gfc, const int gr, const int ch,
                            III_side_info_t * const l3_side);

int     scale_bitcount(const lame_internal_flags * gfc, gr_info * cod_info);

void    huffman_init(lame_internal_flags * const gfc);

void    init_xrpow_core_init(lame_internal_flags * const gfc);

FLOAT   athAdjust(FLOAT a, FLOAT x, FLOAT athFloor, float ATHfixpoint);

#define LARGE_BITS 100000

#endif 
