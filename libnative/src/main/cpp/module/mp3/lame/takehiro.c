



#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "quantize_pvt.h"
#include "tables.h"


static const struct {
    const int region0_count;
    const int region1_count;
} subdv_table[23] = {
    {
    0, 0},              
    {
    0, 0},              
    {
    0, 0},              
    {
    0, 0},              
    {
    0, 0},              
    {
    0, 1},              
    {
    1, 1},              
    {
    1, 1},              
    {
    1, 2},              
    {
    2, 2},              
    {
    2, 3},              
    {
    2, 3},              
    {
    3, 4},              
    {
    3, 4},              
    {
    3, 4},              
    {
    4, 5},              
    {
    4, 5},              
    {
    4, 6},              
    {
    5, 6},              
    {
    5, 6},              
    {
    5, 7},              
    {
    6, 7},              
    {
    6, 7},              
};











static void
quantize_lines_xrpow_01(unsigned int l, FLOAT istep, const FLOAT * xr, int *ix)
{
    const FLOAT compareval0 = (1.0f - 0.4054f) / istep;
    unsigned int i;

    assert(l > 0);
    assert(l % 2 == 0);
    for (i = 0; i < l; i += 2) {
        FLOAT const xr_0 = xr[i+0];
        FLOAT const xr_1 = xr[i+1];
        int const ix_0 = (compareval0 > xr_0) ? 0 : 1;
        int const ix_1 = (compareval0 > xr_1) ? 0 : 1;
        ix[i+0] = ix_0;
        ix[i+1] = ix_1;
    }
}



#ifdef TAKEHIRO_IEEE754_HACK

typedef union {
    float   f;
    int     i;
} fi_union;

#define MAGIC_FLOAT (65536*(128))
#define MAGIC_INT 0x4b000000


static void
quantize_lines_xrpow(unsigned int l, FLOAT istep, const FLOAT * xp, int *pi)
{
    fi_union *fi;
    unsigned int remaining;

    assert(l > 0);

    fi = (fi_union *) pi;

    l = l >> 1;
    remaining = l % 2;
    l = l >> 1;
    while (l--) {
        double  x0 = istep * xp[0];
        double  x1 = istep * xp[1];
        double  x2 = istep * xp[2];
        double  x3 = istep * xp[3];

        x0 += MAGIC_FLOAT;
        fi[0].f = x0;
        x1 += MAGIC_FLOAT;
        fi[1].f = x1;
        x2 += MAGIC_FLOAT;
        fi[2].f = x2;
        x3 += MAGIC_FLOAT;
        fi[3].f = x3;

        fi[0].f = x0 + adj43asm[fi[0].i - MAGIC_INT];
        fi[1].f = x1 + adj43asm[fi[1].i - MAGIC_INT];
        fi[2].f = x2 + adj43asm[fi[2].i - MAGIC_INT];
        fi[3].f = x3 + adj43asm[fi[3].i - MAGIC_INT];

        fi[0].i -= MAGIC_INT;
        fi[1].i -= MAGIC_INT;
        fi[2].i -= MAGIC_INT;
        fi[3].i -= MAGIC_INT;
        fi += 4;
        xp += 4;
    };
    if (remaining) {
        double  x0 = istep * xp[0];
        double  x1 = istep * xp[1];

        x0 += MAGIC_FLOAT;
        fi[0].f = x0;
        x1 += MAGIC_FLOAT;
        fi[1].f = x1;

        fi[0].f = x0 + adj43asm[fi[0].i - MAGIC_INT];
        fi[1].f = x1 + adj43asm[fi[1].i - MAGIC_INT];

        fi[0].i -= MAGIC_INT;
        fi[1].i -= MAGIC_INT;
    }

}


#else


#define XRPOW_FTOI(src,dest) ((dest) = (int)(src))
#define QUANTFAC(rx)  adj43[rx]
#define ROUNDFAC 0.4054


static void
quantize_lines_xrpow(unsigned int l, FLOAT istep, const FLOAT * xr, int *ix)
{
    unsigned int remaining;

    assert(l > 0);

    l = l >> 1;
    remaining = l % 2;
    l = l >> 1;
    while (l--) {
        FLOAT   x0, x1, x2, x3;
        int     rx0, rx1, rx2, rx3;

        x0 = *xr++ * istep;
        x1 = *xr++ * istep;
        XRPOW_FTOI(x0, rx0);
        x2 = *xr++ * istep;
        XRPOW_FTOI(x1, rx1);
        x3 = *xr++ * istep;
        XRPOW_FTOI(x2, rx2);
        x0 += QUANTFAC(rx0);
        XRPOW_FTOI(x3, rx3);
        x1 += QUANTFAC(rx1);
        XRPOW_FTOI(x0, *ix++);
        x2 += QUANTFAC(rx2);
        XRPOW_FTOI(x1, *ix++);
        x3 += QUANTFAC(rx3);
        XRPOW_FTOI(x2, *ix++);
        XRPOW_FTOI(x3, *ix++);
    };
    if (remaining) {
        FLOAT   x0, x1;
        int     rx0, rx1;

        x0 = *xr++ * istep;
        x1 = *xr++ * istep;
        XRPOW_FTOI(x0, rx0);
        XRPOW_FTOI(x1, rx1);
        x0 += QUANTFAC(rx0);
        x1 += QUANTFAC(rx1);
        XRPOW_FTOI(x0, *ix++);
        XRPOW_FTOI(x1, *ix++);
    }

}



#endif





static void
quantize_xrpow(const FLOAT * xp, int *pi, FLOAT istep, gr_info const *const cod_info,
               calc_noise_data const *prev_noise)
{
    
    int     sfb;
    int     sfbmax;
    int     j = 0;
    int     prev_data_use;
    int    *iData;
    int     accumulate = 0;
    int     accumulate01 = 0;
    int    *acc_iData;
    const FLOAT *acc_xp;

    iData = pi;
    acc_xp = xp;
    acc_iData = iData;


    
    prev_data_use = (prev_noise && (cod_info->global_gain == prev_noise->global_gain));

    if (cod_info->block_type == SHORT_TYPE)
        sfbmax = 38;
    else
        sfbmax = 21;

    for (sfb = 0; sfb <= sfbmax; sfb++) {
        int     step = -1;

        if (prev_data_use || cod_info->block_type == NORM_TYPE) {
            step =
                cod_info->global_gain
                - ((cod_info->scalefac[sfb] + (cod_info->preflag ? pretab[sfb] : 0))
                   << (cod_info->scalefac_scale + 1))
                - cod_info->subblock_gain[cod_info->window[sfb]] * 8;
        }
        assert(cod_info->width[sfb] >= 0);
        if (prev_data_use && (prev_noise->step[sfb] == step)) {
            
            if (accumulate) {
                quantize_lines_xrpow(accumulate, istep, acc_xp, acc_iData);
                accumulate = 0;
            }
            if (accumulate01) {
                quantize_lines_xrpow_01(accumulate01, istep, acc_xp, acc_iData);
                accumulate01 = 0;
            }
        }
        else {          
            int     l;
            l = cod_info->width[sfb];

            if ((j + cod_info->width[sfb]) > cod_info->max_nonzero_coeff) {
                
                int     usefullsize;
                usefullsize = cod_info->max_nonzero_coeff - j + 1;
                memset(&pi[cod_info->max_nonzero_coeff], 0,
                       sizeof(int) * (576 - cod_info->max_nonzero_coeff));
                l = usefullsize;

                if (l < 0) {
                    l = 0;
                }

                
                sfb = sfbmax + 1;
            }

            
            if (!accumulate && !accumulate01) {
                acc_iData = iData;
                acc_xp = xp;
            }
            if (prev_noise &&
                prev_noise->sfb_count1 > 0 &&
                sfb >= prev_noise->sfb_count1 &&
                prev_noise->step[sfb] > 0 && step >= prev_noise->step[sfb]) {

                if (accumulate) {
                    quantize_lines_xrpow(accumulate, istep, acc_xp, acc_iData);
                    accumulate = 0;
                    acc_iData = iData;
                    acc_xp = xp;
                }
                accumulate01 += l;
            }
            else {
                if (accumulate01) {
                    quantize_lines_xrpow_01(accumulate01, istep, acc_xp, acc_iData);
                    accumulate01 = 0;
                    acc_iData = iData;
                    acc_xp = xp;
                }
                accumulate += l;
            }

            if (l <= 0) {
                
                if (accumulate01) {
                    quantize_lines_xrpow_01(accumulate01, istep, acc_xp, acc_iData);
                    accumulate01 = 0;
                }
                if (accumulate) {
                    quantize_lines_xrpow(accumulate, istep, acc_xp, acc_iData);
                    accumulate = 0;
                }

                break;  
            }
        }
        if (sfb <= sfbmax) {
            iData += cod_info->width[sfb];
            xp += cod_info->width[sfb];
            j += cod_info->width[sfb];
        }
    }
    if (accumulate) {   
        quantize_lines_xrpow(accumulate, istep, acc_xp, acc_iData);
        accumulate = 0;
    }
    if (accumulate01) { 
        quantize_lines_xrpow_01(accumulate01, istep, acc_xp, acc_iData);
        accumulate01 = 0;
    }

}








static int
ix_max(const int *ix, const int *end)
{
    int     max1 = 0, max2 = 0;

    do {
        int const x1 = *ix++;
        int const x2 = *ix++;
        if (max1 < x1)
            max1 = x1;

        if (max2 < x2)
            max2 = x2;
    } while (ix < end);
    if (max1 < max2)
        max1 = max2;
    return max1;
}








static int
count_bit_ESC(const int *ix, const int *const end, int t1, const int t2, unsigned int *const s)
{
    
    unsigned int const linbits = ht[t1].xlen * 65536u + ht[t2].xlen;
    unsigned int sum = 0, sum2;

    do {
        unsigned int x = *ix++;
        unsigned int y = *ix++;

        if (x >= 15u) {
            x = 15u;
            sum += linbits;
        }
        if (y >= 15u) {
            y = 15u;
            sum += linbits;
        }
        x <<= 4u;
        x += y;
        sum += largetbl[x];
    } while (ix < end);

    sum2 = sum & 0xffffu;
    sum >>= 16u;

    if (sum > sum2) {
        sum = sum2;
        t1 = t2;
    }

    *s += sum;
    return t1;
}


static int
count_bit_noESC(const int *ix, const int *end, int mx, unsigned int *s)
{
    
    unsigned int sum1 = 0;
    const uint8_t *const hlen1 = ht[1].hlen;
    (void) mx;

    do {
        unsigned int const x0 = *ix++;
        unsigned int const x1 = *ix++;
        sum1 += hlen1[ x0+x0 + x1 ];
    } while (ix < end);

    *s += sum1;
    return 1;
}


static const int huf_tbl_noESC[] = {
    1, 2, 5, 7, 7, 10, 10, 13, 13, 13, 13, 13, 13, 13, 13
};


static int
count_bit_noESC_from2(const int *ix, const int *end, int max, unsigned int *s)
{
    int t1 = huf_tbl_noESC[max - 1];
    
    const unsigned int xlen = ht[t1].xlen;
    uint32_t const* table = (t1 == 2) ? &table23[0] : &table56[0];
    unsigned int sum = 0, sum2;

    do {
        unsigned int const x0 = *ix++;
        unsigned int const x1 = *ix++;
        sum += table[ x0 * xlen + x1 ];
    } while (ix < end);

    sum2 = sum & 0xffffu;
    sum >>= 16u;

    if (sum > sum2) {
        sum = sum2;
        t1++;
    }

    *s += sum;
    return t1;
}


inline static int
count_bit_noESC_from3(const int *ix, const int *end, int max, unsigned int * s)
{
    int t1 = huf_tbl_noESC[max - 1];
    
    unsigned int sum1 = 0;
    unsigned int sum2 = 0;
    unsigned int sum3 = 0;
    const unsigned int xlen = ht[t1].xlen;
    const uint8_t *const hlen1 = ht[t1].hlen;
    const uint8_t *const hlen2 = ht[t1 + 1].hlen;
    const uint8_t *const hlen3 = ht[t1 + 2].hlen;
    int     t;

    do {
        unsigned int x0 = *ix++;
        unsigned int x1 = *ix++;
        unsigned int x = x0 * xlen + x1;
        sum1 += hlen1[x];
        sum2 += hlen2[x];
        sum3 += hlen3[x];
    } while (ix < end);

    t = t1;
    if (sum1 > sum2) {
        sum1 = sum2;
        t++;
    }
    if (sum1 > sum3) {
        sum1 = sum3;
        t = t1 + 2;
    }
    *s += sum1;

    return t;  
}







static int count_bit_null(const int* ix, const int* end, int max, unsigned int* s)
{
    (void) ix;
    (void) end;
    (void) max;
    (void) s;
    return 0;
}

typedef int (*count_fnc)(const int* ix, const int* end, int max, unsigned int* s);
  
static count_fnc count_fncs[] = 
{ &count_bit_null
, &count_bit_noESC
, &count_bit_noESC_from2
, &count_bit_noESC_from2
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
, &count_bit_noESC_from3
};

static int
choose_table_nonMMX(const int *ix, const int *const end, int *const _s)
{
    unsigned int* s = (unsigned int*)_s;
    unsigned int  max;
    int     choice, choice2;
    max = ix_max(ix, end);

    if (max <= 15) {
      return count_fncs[max](ix, end, max, s);
    }
    
    if (max > IXMAX_VAL) {
        *s = LARGE_BITS;
        return -1;
    }
    max -= 15u;
    for (choice2 = 24; choice2 < 32; choice2++) {
        if (ht[choice2].linmax >= max) {
            break;
        }
    }

    for (choice = choice2 - 8; choice < 24; choice++) {
        if (ht[choice].linmax >= max) {
            break;
        }
    }
    return count_bit_ESC(ix, end, choice, choice2, s);
}






int
noquant_count_bits(lame_internal_flags const *const gfc,
                   gr_info * const gi, calc_noise_data * prev_noise)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     bits = 0;
    int     i, a1, a2;
    int const *const ix = gi->l3_enc;

    i = Min(576, ((gi->max_nonzero_coeff + 2) >> 1) << 1);

    if (prev_noise)
        prev_noise->sfb_count1 = 0;

    
    for (; i > 1; i -= 2)
        if (ix[i - 1] | ix[i - 2])
            break;
    gi->count1 = i;

    
    a1 = a2 = 0;
    for (; i > 3; i -= 4) {
        int x4 = ix[i-4];
        int x3 = ix[i-3];
        int x2 = ix[i-2];
        int x1 = ix[i-1];
        int     p;
        
        if ((unsigned int) (x4 | x3 | x2 | x1) > 1)
            break;

        p = ((x4 * 2 + x3) * 2 + x2) * 2 + x1;
        a1 += t32l[p];
        a2 += t33l[p];
    }

    bits = a1;
    gi->count1table_select = 0;
    if (a1 > a2) {
        bits = a2;
        gi->count1table_select = 1;
    }

    gi->count1bits = bits;
    gi->big_values = i;
    if (i == 0)
        return bits;

    if (gi->block_type == SHORT_TYPE) {
        a1 = 3 * gfc->scalefac_band.s[3];
        if (a1 > gi->big_values)
            a1 = gi->big_values;
        a2 = gi->big_values;

    }
    else if (gi->block_type == NORM_TYPE) {
        assert(i <= 576); 
        a1 = gi->region0_count = gfc->sv_qnt.bv_scf[i - 2];
        a2 = gi->region1_count = gfc->sv_qnt.bv_scf[i - 1];

        assert(a1 + a2 + 2 < SBPSY_l);
        a2 = gfc->scalefac_band.l[a1 + a2 + 2];
        a1 = gfc->scalefac_band.l[a1 + 1];
        if (a2 < i)
            gi->table_select[2] = gfc->choose_table(ix + a2, ix + i, &bits);

    }
    else {
        gi->region0_count = 7;
        
        gi->region1_count = SBMAX_l - 1 - 7 - 1;
        a1 = gfc->scalefac_band.l[7 + 1];
        a2 = i;
        if (a1 > a2) {
            a1 = a2;
        }
    }


    
    
    a1 = Min(a1, i);
    a2 = Min(a2, i);

    assert(a1 >= 0);
    assert(a2 >= 0);

    
    if (0 < a1)
        gi->table_select[0] = gfc->choose_table(ix, ix + a1, &bits);
    if (a1 < a2)
        gi->table_select[1] = gfc->choose_table(ix + a1, ix + a2, &bits);
    if (cfg->use_best_huffman == 2) {
        gi->part2_3_length = bits;
        best_huffman_divide(gfc, gi);
        bits = gi->part2_3_length;
    }


    if (prev_noise) {
        if (gi->block_type == NORM_TYPE) {
            int     sfb = 0;
            while (gfc->scalefac_band.l[sfb] < gi->big_values) {
                sfb++;
            }
            prev_noise->sfb_count1 = sfb;
        }
    }

    return bits;
}

int
count_bits(lame_internal_flags const *const gfc,
           const FLOAT * const xr, gr_info * const gi, calc_noise_data * prev_noise)
{
    int    *const ix = gi->l3_enc;

    
    FLOAT const w = (IXMAX_VAL) / IPOW20(gi->global_gain);

    if (gi->xrpow_max > w)
        return LARGE_BITS;

    quantize_xrpow(xr, ix, IPOW20(gi->global_gain), gi, prev_noise);

    if (gfc->sv_qnt.substep_shaping & 2) {
        int     sfb, j = 0;
        
        int const gain = gi->global_gain + gi->scalefac_scale;
        const FLOAT roundfac = 0.634521682242439 / IPOW20(gain);
        for (sfb = 0; sfb < gi->sfbmax; sfb++) {
            int const width = gi->width[sfb];
            assert(width >= 0);
            if (!gfc->sv_qnt.pseudohalf[sfb]) {
                j += width;
            }
            else {
                int     k;
                for (k = j, j += width; k < j; ++k) {
                    ix[k] = (xr[k] >= roundfac) ? ix[k] : 0;
                }
            }
        }
    }
    return noquant_count_bits(gfc, gi, prev_noise);
}




inline static void
recalc_divide_init(const lame_internal_flags * const gfc,
                   gr_info const *cod_info,
                   int const *const ix, int r01_bits[], int r01_div[], int r0_tbl[], int r1_tbl[])
{
    int     r0, r1, bigv, r0t, r1t, bits;

    bigv = cod_info->big_values;

    for (r0 = 0; r0 <= 7 + 15; r0++) {
        r01_bits[r0] = LARGE_BITS;
    }

    for (r0 = 0; r0 < 16; r0++) {
        int const a1 = gfc->scalefac_band.l[r0 + 1];
        int     r0bits;
        if (a1 >= bigv)
            break;
        r0bits = 0;
        r0t = gfc->choose_table(ix, ix + a1, &r0bits);

        for (r1 = 0; r1 < 8; r1++) {
            int const a2 = gfc->scalefac_band.l[r0 + r1 + 2];
            if (a2 >= bigv)
                break;

            bits = r0bits;
            r1t = gfc->choose_table(ix + a1, ix + a2, &bits);
            if (r01_bits[r0 + r1] > bits) {
                r01_bits[r0 + r1] = bits;
                r01_div[r0 + r1] = r0;
                r0_tbl[r0 + r1] = r0t;
                r1_tbl[r0 + r1] = r1t;
            }
        }
    }
}

inline static void
recalc_divide_sub(const lame_internal_flags * const gfc,
                  const gr_info * cod_info2,
                  gr_info * const gi,
                  const int *const ix,
                  const int r01_bits[], const int r01_div[], const int r0_tbl[], const int r1_tbl[])
{
    int     bits, r2, a2, bigv, r2t;

    bigv = cod_info2->big_values;

    for (r2 = 2; r2 < SBMAX_l + 1; r2++) {
        a2 = gfc->scalefac_band.l[r2];
        if (a2 >= bigv)
            break;

        bits = r01_bits[r2 - 2] + cod_info2->count1bits;
        if (gi->part2_3_length <= bits)
            break;

        r2t = gfc->choose_table(ix + a2, ix + bigv, &bits);
        if (gi->part2_3_length <= bits)
            continue;

        memcpy(gi, cod_info2, sizeof(gr_info));
        gi->part2_3_length = bits;
        gi->region0_count = r01_div[r2 - 2];
        gi->region1_count = r2 - 2 - r01_div[r2 - 2];
        gi->table_select[0] = r0_tbl[r2 - 2];
        gi->table_select[1] = r1_tbl[r2 - 2];
        gi->table_select[2] = r2t;
    }
}




void
best_huffman_divide(const lame_internal_flags * const gfc, gr_info * const gi)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     i, a1, a2;
    gr_info cod_info2;
    int const *const ix = gi->l3_enc;

    int     r01_bits[7 + 15 + 1];
    int     r01_div[7 + 15 + 1];
    int     r0_tbl[7 + 15 + 1];
    int     r1_tbl[7 + 15 + 1];


    
    if (gi->block_type == SHORT_TYPE && cfg->mode_gr == 1)
        return;


    memcpy(&cod_info2, gi, sizeof(gr_info));
    if (gi->block_type == NORM_TYPE) {
        recalc_divide_init(gfc, gi, ix, r01_bits, r01_div, r0_tbl, r1_tbl);
        recalc_divide_sub(gfc, &cod_info2, gi, ix, r01_bits, r01_div, r0_tbl, r1_tbl);
    }

    i = cod_info2.big_values;
    if (i == 0 || (unsigned int) (ix[i - 2] | ix[i - 1]) > 1)
        return;

    i = gi->count1 + 2;
    if (i > 576)
        return;

    
    memcpy(&cod_info2, gi, sizeof(gr_info));
    cod_info2.count1 = i;
    a1 = a2 = 0;

    assert(i <= 576);

    for (; i > cod_info2.big_values; i -= 4) {
        int const p = ((ix[i - 4] * 2 + ix[i - 3]) * 2 + ix[i - 2]) * 2 + ix[i - 1];
        a1 += t32l[p];
        a2 += t33l[p];
    }
    cod_info2.big_values = i;

    cod_info2.count1table_select = 0;
    if (a1 > a2) {
        a1 = a2;
        cod_info2.count1table_select = 1;
    }

    cod_info2.count1bits = a1;

    if (cod_info2.block_type == NORM_TYPE)
        recalc_divide_sub(gfc, &cod_info2, gi, ix, r01_bits, r01_div, r0_tbl, r1_tbl);
    else {
        
        cod_info2.part2_3_length = a1;
        a1 = gfc->scalefac_band.l[7 + 1];
        if (a1 > i) {
            a1 = i;
        }
        if (a1 > 0)
            cod_info2.table_select[0] =
                gfc->choose_table(ix, ix + a1, (int *) &cod_info2.part2_3_length);
        if (i > a1)
            cod_info2.table_select[1] =
                gfc->choose_table(ix + a1, ix + i, (int *) &cod_info2.part2_3_length);
        if (gi->part2_3_length > cod_info2.part2_3_length)
            memcpy(gi, &cod_info2, sizeof(gr_info));
    }
}

static const int slen1_n[16] = { 1, 1, 1, 1, 8, 2, 2, 2, 4, 4, 4, 8, 8, 8, 16, 16 };
static const int slen2_n[16] = { 1, 2, 4, 8, 1, 2, 4, 8, 2, 4, 8, 2, 4, 8, 4, 8 };
const int slen1_tab[16] = { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 };
const int slen2_tab[16] = { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 };

static void
scfsi_calc(int ch, III_side_info_t * l3_side)
{
    unsigned int i;
    int     s1, s2, c1, c2;
    int     sfb;
    gr_info *const gi = &l3_side->tt[1][ch];
    gr_info const *const g0 = &l3_side->tt[0][ch];

    for (i = 0; i < (sizeof(scfsi_band) / sizeof(int)) - 1; i++) {
        for (sfb = scfsi_band[i]; sfb < scfsi_band[i + 1]; sfb++) {
            if (g0->scalefac[sfb] != gi->scalefac[sfb]
                && gi->scalefac[sfb] >= 0)
                break;
        }
        if (sfb == scfsi_band[i + 1]) {
            for (sfb = scfsi_band[i]; sfb < scfsi_band[i + 1]; sfb++) {
                gi->scalefac[sfb] = -1;
            }
            l3_side->scfsi[ch][i] = 1;
        }
    }

    s1 = c1 = 0;
    for (sfb = 0; sfb < 11; sfb++) {
        if (gi->scalefac[sfb] == -1)
            continue;
        c1++;
        if (s1 < gi->scalefac[sfb])
            s1 = gi->scalefac[sfb];
    }

    s2 = c2 = 0;
    for (; sfb < SBPSY_l; sfb++) {
        if (gi->scalefac[sfb] == -1)
            continue;
        c2++;
        if (s2 < gi->scalefac[sfb])
            s2 = gi->scalefac[sfb];
    }

    for (i = 0; i < 16; i++) {
        if (s1 < slen1_n[i] && s2 < slen2_n[i]) {
            int const c = slen1_tab[i] * c1 + slen2_tab[i] * c2;
            if (gi->part2_length > c) {
                gi->part2_length = c;
                gi->scalefac_compress = (int)i;
            }
        }
    }
}


void
best_scalefac_store(const lame_internal_flags * gfc,
                    const int gr, const int ch, III_side_info_t * const l3_side)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    
    gr_info *const gi = &l3_side->tt[gr][ch];
    int     sfb, i, j, l;
    int     recalc = 0;

    
    
    j = 0;
    for (sfb = 0; sfb < gi->sfbmax; sfb++) {
        int const width = gi->width[sfb];
        assert(width >= 0);
        for (l = j, j += width; l < j; ++l) {
            if (gi->l3_enc[l] != 0)
                break;
        }
        if (l == j)
            gi->scalefac[sfb] = recalc = -2; 
        
    }

    if (!gi->scalefac_scale && !gi->preflag) {
        int     s = 0;
        for (sfb = 0; sfb < gi->sfbmax; sfb++)
            if (gi->scalefac[sfb] > 0)
                s |= gi->scalefac[sfb];

        if (!(s & 1) && s != 0) {
            for (sfb = 0; sfb < gi->sfbmax; sfb++)
                if (gi->scalefac[sfb] > 0)
                    gi->scalefac[sfb] >>= 1;

            gi->scalefac_scale = recalc = 1;
        }
    }

    if (!gi->preflag && gi->block_type != SHORT_TYPE && cfg->mode_gr == 2) {
        for (sfb = 11; sfb < SBPSY_l; sfb++)
            if (gi->scalefac[sfb] < pretab[sfb] && gi->scalefac[sfb] != -2)
                break;
        if (sfb == SBPSY_l) {
            for (sfb = 11; sfb < SBPSY_l; sfb++)
                if (gi->scalefac[sfb] > 0)
                    gi->scalefac[sfb] -= pretab[sfb];

            gi->preflag = recalc = 1;
        }
    }

    for (i = 0; i < 4; i++)
        l3_side->scfsi[ch][i] = 0;

    if (cfg->mode_gr == 2 && gr == 1
        && l3_side->tt[0][ch].block_type != SHORT_TYPE
        && l3_side->tt[1][ch].block_type != SHORT_TYPE) {
        scfsi_calc(ch, l3_side);
        recalc = 0;
    }
    for (sfb = 0; sfb < gi->sfbmax; sfb++) {
        if (gi->scalefac[sfb] == -2) {
            gi->scalefac[sfb] = 0; 
        }
    }
    if (recalc) {
        (void) scale_bitcount(gfc, gi);
    }
}


#ifndef NDEBUG
static int
all_scalefactors_not_negative(int const *scalefac, int n)
{
    int     i;
    for (i = 0; i < n; ++i) {
        if (scalefac[i] < 0)
            return 0;
    }
    return 1;
}
#endif





static const int scale_short[16] = {
    0, 18, 36, 54, 54, 36, 54, 72, 54, 72, 90, 72, 90, 108, 108, 126
};


static const int scale_mixed[16] = {
    0, 18, 36, 54, 51, 35, 53, 71, 52, 70, 88, 69, 87, 105, 104, 122
};


static const int scale_long[16] = {
    0, 10, 20, 30, 33, 21, 31, 41, 32, 42, 52, 43, 53, 63, 64, 74
};








static int
mpeg1_scale_bitcount(const lame_internal_flags * gfc, gr_info * const cod_info)
{
    int     k, sfb, max_slen1 = 0, max_slen2 = 0;

    
    const int *tab;
    int    *const scalefac = cod_info->scalefac;

    (void) gfc;
    assert(all_scalefactors_not_negative(scalefac, cod_info->sfbmax));

    if (cod_info->block_type == SHORT_TYPE) {
        tab = scale_short;
        if (cod_info->mixed_block_flag)
            tab = scale_mixed;
    }
    else {              
        tab = scale_long;
        if (!cod_info->preflag) {
            for (sfb = 11; sfb < SBPSY_l; sfb++)
                if (scalefac[sfb] < pretab[sfb])
                    break;

            if (sfb == SBPSY_l) {
                cod_info->preflag = 1;
                for (sfb = 11; sfb < SBPSY_l; sfb++)
                    scalefac[sfb] -= pretab[sfb];
            }
        }
    }

    for (sfb = 0; sfb < cod_info->sfbdivide; sfb++)
        if (max_slen1 < scalefac[sfb])
            max_slen1 = scalefac[sfb];

    for (; sfb < cod_info->sfbmax; sfb++)
        if (max_slen2 < scalefac[sfb])
            max_slen2 = scalefac[sfb];

    
    cod_info->part2_length = LARGE_BITS;
    for (k = 0; k < 16; k++) {
        if (max_slen1 < slen1_n[k] && max_slen2 < slen2_n[k]
            && cod_info->part2_length > tab[k]) {
            cod_info->part2_length = tab[k];
            cod_info->scalefac_compress = k;
        }
    }
    return cod_info->part2_length == LARGE_BITS;
}




static const int max_range_sfac_tab[6][4] = {
    {15, 15, 7, 7},
    {15, 15, 7, 0},
    {7, 3, 0, 0},
    {15, 31, 31, 0},
    {7, 7, 7, 0},
    {3, 3, 0, 0}
};














static int
mpeg2_scale_bitcount(const lame_internal_flags * gfc, gr_info * const cod_info)
{
    int     table_number, row_in_table, partition, nr_sfb, window, over;
    int     i, sfb, max_sfac[4];
    const int *partition_table;
    int const *const scalefac = cod_info->scalefac;

    
    if (cod_info->preflag)
        table_number = 2;
    else
        table_number = 0;

    for (i = 0; i < 4; i++)
        max_sfac[i] = 0;

    if (cod_info->block_type == SHORT_TYPE) {
        row_in_table = 1;
        partition_table = &nr_of_sfb_block[table_number][row_in_table][0];
        for (sfb = 0, partition = 0; partition < 4; partition++) {
            nr_sfb = partition_table[partition] / 3;
            for (i = 0; i < nr_sfb; i++, sfb++)
                for (window = 0; window < 3; window++)
                    if (scalefac[sfb * 3 + window] > max_sfac[partition])
                        max_sfac[partition] = scalefac[sfb * 3 + window];
        }
    }
    else {
        row_in_table = 0;
        partition_table = &nr_of_sfb_block[table_number][row_in_table][0];
        for (sfb = 0, partition = 0; partition < 4; partition++) {
            nr_sfb = partition_table[partition];
            for (i = 0; i < nr_sfb; i++, sfb++)
                if (scalefac[sfb] > max_sfac[partition])
                    max_sfac[partition] = scalefac[sfb];
        }
    }

    for (over = 0, partition = 0; partition < 4; partition++) {
        if (max_sfac[partition] > max_range_sfac_tab[table_number][partition])
            over++;
    }
    if (!over) {
        
        static const int log2tab[] = { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };

        int     slen1, slen2, slen3, slen4;

        cod_info->sfb_partition_table = nr_of_sfb_block[table_number][row_in_table];
        for (partition = 0; partition < 4; partition++)
            cod_info->slen[partition] = log2tab[max_sfac[partition]];

        
        slen1 = cod_info->slen[0];
        slen2 = cod_info->slen[1];
        slen3 = cod_info->slen[2];
        slen4 = cod_info->slen[3];

        switch (table_number) {
        case 0:
            cod_info->scalefac_compress = (((slen1 * 5) + slen2) << 4)
                + (slen3 << 2)
                + slen4;
            break;

        case 1:
            cod_info->scalefac_compress = 400 + (((slen1 * 5) + slen2) << 2)
                + slen3;
            break;

        case 2:
            cod_info->scalefac_compress = 500 + (slen1 * 3) + slen2;
            break;

        default:
            ERRORF(gfc, "intensity stereo not implemented yet\n");
            break;
        }
    }
#ifdef DEBUG
    if (over)
        ERRORF(gfc, "---WARNING !! Amplification of some bands over limits\n");
#endif
    if (!over) {
        assert(cod_info->sfb_partition_table);
        cod_info->part2_length = 0;
        for (partition = 0; partition < 4; partition++)
            cod_info->part2_length +=
                cod_info->slen[partition] * cod_info->sfb_partition_table[partition];
    }
    return over;
}


int
scale_bitcount(const lame_internal_flags * gfc, gr_info * cod_info)
{
    if (gfc->cfg.mode_gr == 2) {
        return mpeg1_scale_bitcount(gfc, cod_info);
    }
    else {
        return mpeg2_scale_bitcount(gfc, cod_info);
    }
}


#ifdef MMX_choose_table
extern int choose_table_MMX(const int *ix, const int *const end, int *const s);
#endif

void
huffman_init(lame_internal_flags * const gfc)
{
    int     i;

    gfc->choose_table = choose_table_nonMMX;

#ifdef MMX_choose_table
    if (gfc->CPU_features.MMX) {
        gfc->choose_table = choose_table_MMX;
    }
#endif

    for (i = 2; i <= 576; i += 2) {
        int     scfb_anz = 0, bv_index;
        while (gfc->scalefac_band.l[++scfb_anz] < i);

        bv_index = subdv_table[scfb_anz].region0_count;
        while (gfc->scalefac_band.l[bv_index + 1] > i)
            bv_index--;

        if (bv_index < 0) {
            
            bv_index = subdv_table[scfb_anz].region0_count;
        }

        gfc->sv_qnt.bv_scf[i - 2] = bv_index;

        bv_index = subdv_table[scfb_anz].region1_count;
        while (gfc->scalefac_band.l[bv_index + gfc->sv_qnt.bv_scf[i - 2] + 2] > i)
            bv_index--;

        if (bv_index < 0) {
            bv_index = subdv_table[scfb_anz].region1_count;
        }

        gfc->sv_qnt.bv_scf[i - 1] = bv_index;
    }
}
