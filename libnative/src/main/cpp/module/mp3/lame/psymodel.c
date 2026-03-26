












#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <android/legacy_stdlib_inlines.h>
#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "psymodel.h"
#include "lame_global_flags.h"
#include "fft.h"
#include "lame-analysis.h"


#define NSFIRLEN 21

#ifdef M_LN10
#define  LN_TO_LOG10  (M_LN10/10)
#else
#define  LN_TO_LOG10  0.2302585093
#endif









static  FLOAT
psycho_loudness_approx(FLOAT const *energy, FLOAT const *eql_w)
{
    int     i;
    FLOAT   loudness_power;

    loudness_power = 0.0;
    
    for (i = 0; i < BLKSIZE / 2; ++i)
        loudness_power += energy[i] * eql_w[i];
    loudness_power *= VO_SCALE;

    return loudness_power;
}







#define I1LIMIT 8       
#define I2LIMIT 23      
#define MLIMIT  15      

static FLOAT ma_max_i1;
static FLOAT ma_max_i2;
static FLOAT ma_max_m;

    
static const FLOAT tab[] = {
    1.0  ,
    0.79433  ,
    0.63096  ,
    0.63096  ,
    0.63096  ,
    0.63096  ,
    0.63096  ,
    0.25119  ,
    0.11749             
};

static const int tab_mask_add_delta[] = { 2, 2, 2, 1, 1, 1, 0, 0, -1 };
#define STATIC_ASSERT_EQUAL_DIMENSION(A,B) {extern char static_assert_##A[dimension_of(A) == dimension_of(B) ? 1 : -1];(void) static_assert_##A;}

inline static int
mask_add_delta(int i)
{
    STATIC_ASSERT_EQUAL_DIMENSION(tab_mask_add_delta,tab);
    assert(i < (int)dimension_of(tab));
    return tab_mask_add_delta[i];
}


static void
init_mask_add_max_values(void)
{
    ma_max_i1 = pow(10, (I1LIMIT + 1) / 16.0);
    ma_max_i2 = pow(10, (I2LIMIT + 1) / 16.0);
    ma_max_m = pow(10, (MLIMIT) / 10.0);
}





inline static FLOAT
vbrpsy_mask_add(FLOAT m1, FLOAT m2, int b, int delta)
{
    static const FLOAT table2[] = {
        1.33352 * 1.33352, 1.35879 * 1.35879, 1.38454 * 1.38454, 1.39497 * 1.39497,
        1.40548 * 1.40548, 1.3537 * 1.3537, 1.30382 * 1.30382, 1.22321 * 1.22321,
        1.14758 * 1.14758,
        1
    };

    FLOAT   ratio;

    if (m1 < 0) {
        m1 = 0;
    }
    if (m2 < 0) {
        m2 = 0;
    }
    if (m1 <= 0) {
        return m2;
    }
    if (m2 <= 0) {
        return m1;
    }
    if (m2 > m1) {
        ratio = m2 / m1;
    }
    else {
        ratio = m1 / m2;
    }
    if (abs(b) <= delta) {       
        
        if (ratio >= ma_max_i1) {
            return m1 + m2;
        }
        else {
            int     i = (int) (FAST_LOG10_X(ratio, 16.0f));
            return (m1 + m2) * table2[i];
        }
    }
    if (ratio < ma_max_i2) {
        return m1 + m2;
    }
    if (m1 < m2) {
        m1 = m2;
    }
    return m1;
}



static void
convert_partition2scalefac(PsyConst_CB2SB_t const *const gd, FLOAT const *eb, FLOAT const *thr,
                           FLOAT enn_out[], FLOAT thm_out[])
{
    FLOAT   enn, thmm;
    int     sb, b, n = gd->n_sb;
    enn = thmm = 0.0f;
    for (sb = b = 0; sb < n; ++b, ++sb) {
        int const bo_sb = gd->bo[sb];
        int const npart = gd->npart;
        int const b_lim = bo_sb < npart ? bo_sb : npart;
        while (b < b_lim) {
            assert(eb[b] >= 0); 
            assert(thr[b] >= 0);
            enn += eb[b];
            thmm += thr[b];
            b++;
        }
        if (b >= npart) {
            enn_out[sb] = enn;
            thm_out[sb] = thmm;
            ++sb;
            break;
        }
        assert(eb[b] >= 0); 
        assert(thr[b] >= 0);
        {
            
            FLOAT const w_curr = gd->bo_weight[sb];
            FLOAT const w_next = 1.0f - w_curr;
            enn += w_curr * eb[b];
            thmm += w_curr * thr[b];
            enn_out[sb] = enn;
            thm_out[sb] = thmm;
            enn = w_next * eb[b];
            thmm = w_next * thr[b];
        }
    }
    
    for (; sb < n; ++sb) {
        enn_out[sb] = 0;
        thm_out[sb] = 0;
    }
}

static void
convert_partition2scalefac_s(lame_internal_flags * gfc, FLOAT const *eb, FLOAT const *thr, int chn,
                             int sblock)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->s;
    FLOAT   enn[SBMAX_s], thm[SBMAX_s];
    int     sb;
    convert_partition2scalefac(gds, eb, thr, enn, thm);
    for (sb = 0; sb < SBMAX_s; ++sb) {
        psv->en[chn].s[sb][sblock] = enn[sb];
        psv->thm[chn].s[sb][sblock] = thm[sb];
    }
}


static void
convert_partition2scalefac_l(lame_internal_flags * gfc, FLOAT const *eb, FLOAT const *thr, int chn)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gdl = &gfc->cd_psy->l;
    FLOAT  *enn = &psv->en[chn].l[0];
    FLOAT  *thm = &psv->thm[chn].l[0];
    convert_partition2scalefac(gdl, eb, thr, enn, thm);
}

static void
convert_partition2scalefac_l_to_s(lame_internal_flags * gfc, FLOAT const *eb, FLOAT const *thr,
                                  int chn)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->l_to_s;
    FLOAT   enn[SBMAX_s], thm[SBMAX_s];
    int     sb, sblock;
    convert_partition2scalefac(gds, eb, thr, enn, thm);
    for (sb = 0; sb < SBMAX_s; ++sb) {
        FLOAT const scale = 1. / 64.f;
        FLOAT const tmp_enn = enn[sb];
        FLOAT const tmp_thm = thm[sb] * scale;
        for (sblock = 0; sblock < 3; ++sblock) {
            psv->en[chn].s[sb][sblock] = tmp_enn;
            psv->thm[chn].s[sb][sblock] = tmp_thm;
        }
    }
}



static inline FLOAT
NS_INTERP(FLOAT x, FLOAT y, FLOAT r)
{
    
    if (r >= 1.0f)
        return x;       
    if (r <= 0.0f)
        return y;
    if (y > 0.0f)
        return powf(x / y, r) * y; 
    return 0.0f;        
}



static  FLOAT
pecalc_s(III_psy_ratio const *mr, FLOAT masking_lower)
{
    FLOAT   pe_s;
    static const FLOAT regcoef_s[] = {
        11.8,           
        13.6,
        17.2,
        32,
        46.5,
        51.3,
        57.5,
        67.1,
        71.5,
        84.6,
        97.6,
        130,

    };
    unsigned int sb, sblock;

    pe_s = 1236.28f / 4;
    for (sb = 0; sb < SBMAX_s - 1; sb++) {
        for (sblock = 0; sblock < 3; sblock++) {
            FLOAT const thm = mr->thm.s[sb][sblock];
            assert(sb < dimension_of(regcoef_s));
            if (thm > 0.0f) {
                FLOAT const x = thm * masking_lower;
                FLOAT const en = mr->en.s[sb][sblock];
                if (en > x) {
                    if (en > x * 1e10f) {
                        pe_s += regcoef_s[sb] * (10.0f * LOG10);
                    }
                    else {
                        assert(x > 0);
                        pe_s += regcoef_s[sb] * FAST_LOG10(en / x);
                    }
                }
            }
        }
    }

    return pe_s;
}

static  FLOAT
pecalc_l(III_psy_ratio const *mr, FLOAT masking_lower)
{
    FLOAT   pe_l;
    static const FLOAT regcoef_l[] = {
        6.8,            
        5.8,
        5.8,
        6.4,
        6.5,
        9.9,
        12.1,
        14.4,
        15,
        18.9,
        21.6,
        26.9,
        34.2,
        40.2,
        46.8,
        56.5,
        60.7,
        73.9,
        85.7,
        93.4,
        126.1,

    };
    unsigned int sb;

    pe_l = 1124.23f / 4;
    for (sb = 0; sb < SBMAX_l - 1; sb++) {
        FLOAT const thm = mr->thm.l[sb];
        assert(sb < dimension_of(regcoef_l));
        if (thm > 0.0f) {
            FLOAT const x = thm * masking_lower;
            FLOAT const en = mr->en.l[sb];
            if (en > x) {
                if (en > x * 1e10f) {
                    pe_l += regcoef_l[sb] * (10.0f * LOG10);
                }
                else {
                    assert(x > 0);
                    pe_l += regcoef_l[sb] * FAST_LOG10(en / x);
                }
            }
        }
    }

    return pe_l;
}


static void
calc_energy(PsyConst_CB2SB_t const *l, FLOAT const *fftenergy, FLOAT * eb, FLOAT * max, FLOAT * avg)
{
    int     b, j;

    for (b = j = 0; b < l->npart; ++b) {
        FLOAT   ebb = 0, m = 0;
        int     i;
        for (i = 0; i < l->numlines[b]; ++i, ++j) {
            FLOAT const el = fftenergy[j];
            assert(el >= 0);
            ebb += el;
            if (m < el)
                m = el;
        }
        eb[b] = ebb;
        max[b] = m;
        avg[b] = ebb * l->rnumlines[b];
        assert(l->rnumlines[b] >= 0);
        assert(ebb >= 0);
        assert(eb[b] >= 0);
        assert(max[b] >= 0);
        assert(avg[b] >= 0);
    }
}


static void
calc_mask_index_l(lame_internal_flags const *gfc, FLOAT const *max,
                  FLOAT const *avg, unsigned char *mask_idx)
{
    PsyConst_CB2SB_t const *const gdl = &gfc->cd_psy->l;
    FLOAT   m, a;
    int     b, k;
    int const last_tab_entry = sizeof(tab) / sizeof(tab[0]) - 1;
    b = 0;
    a = avg[b] + avg[b + 1];
    assert(a >= 0);
    if (a > 0.0f) {
        m = max[b];
        if (m < max[b + 1])
            m = max[b + 1];
        assert((gdl->numlines[b] + gdl->numlines[b + 1] - 1) > 0);
        a = 20.0f * (m * 2.0f - a)
            / (a * (gdl->numlines[b] + gdl->numlines[b + 1] - 1));
        k = (int) a;
        if (k > last_tab_entry)
            k = last_tab_entry;
        mask_idx[b] = k;
    }
    else {
        mask_idx[b] = 0;
    }

    for (b = 1; b < gdl->npart - 1; b++) {
        a = avg[b - 1] + avg[b] + avg[b + 1];
        assert(a >= 0);
        if (a > 0.0f) {
            m = max[b - 1];
            if (m < max[b])
                m = max[b];
            if (m < max[b + 1])
                m = max[b + 1];
            assert((gdl->numlines[b - 1] + gdl->numlines[b] + gdl->numlines[b + 1] - 1) > 0);
            a = 20.0f * (m * 3.0f - a)
                / (a * (gdl->numlines[b - 1] + gdl->numlines[b] + gdl->numlines[b + 1] - 1));
            k = (int) a;
            if (k > last_tab_entry)
                k = last_tab_entry;
            mask_idx[b] = k;
        }
        else {
            mask_idx[b] = 0;
        }
    }
    assert(b > 0);
    assert(b == gdl->npart - 1);

    a = avg[b - 1] + avg[b];
    assert(a >= 0);
    if (a > 0.0f) {
        m = max[b - 1];
        if (m < max[b])
            m = max[b];
        assert((gdl->numlines[b - 1] + gdl->numlines[b] - 1) > 0);
        a = 20.0f * (m * 2.0f - a)
            / (a * (gdl->numlines[b - 1] + gdl->numlines[b] - 1));
        k = (int) a;
        if (k > last_tab_entry)
            k = last_tab_entry;
        mask_idx[b] = k;
    }
    else {
        mask_idx[b] = 0;
    }
    assert(b == (gdl->npart - 1));
}


static void
vbrpsy_compute_fft_l(lame_internal_flags * gfc, const sample_t * const buffer[2], int chn,
                     int gr_out, FLOAT fftenergy[HBLKSIZE], FLOAT(*wsamp_l)[BLKSIZE])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    PsyStateVar_t *psv = &gfc->sv_psy;
    plotting_data *plt = cfg->analysis ? gfc->pinfo : 0;
    int     j;

    if (chn < 2) {
        fft_long(gfc, *wsamp_l, chn, buffer);
    }
    else if (chn == 2) {
        FLOAT const sqrt2_half = SQRT2 * 0.5f;
        
        for (j = BLKSIZE - 1; j >= 0; --j) {
            FLOAT const l = wsamp_l[0][j];
            FLOAT const r = wsamp_l[1][j];
            wsamp_l[0][j] = (l + r) * sqrt2_half;
            wsamp_l[1][j] = (l - r) * sqrt2_half;
        }
    }

    
    fftenergy[0] = wsamp_l[0][0];
    fftenergy[0] *= fftenergy[0];

    for (j = BLKSIZE / 2 - 1; j >= 0; --j) {
        FLOAT const re = (*wsamp_l)[BLKSIZE / 2 - j];
        FLOAT const im = (*wsamp_l)[BLKSIZE / 2 + j];
        fftenergy[BLKSIZE / 2 - j] = (re * re + im * im) * 0.5f;
    }
    
    {
        FLOAT   totalenergy = 0.0f;
        for (j = 11; j < HBLKSIZE; j++)
            totalenergy += fftenergy[j];

        psv->tot_ener[chn] = totalenergy;
    }

    if (plt) {
        for (j = 0; j < HBLKSIZE; j++) {
            plt->energy[gr_out][chn][j] = plt->energy_save[chn][j];
            plt->energy_save[chn][j] = fftenergy[j];
        }
    }
}


static void
vbrpsy_compute_fft_s(lame_internal_flags const *gfc, const sample_t * const buffer[2], int chn,
                     int sblock, FLOAT(*fftenergy_s)[HBLKSIZE_s], FLOAT(*wsamp_s)[3][BLKSIZE_s])
{
    int     j;

    if (sblock == 0 && chn < 2) {
        fft_short(gfc, *wsamp_s, chn, buffer);
    }
    if (chn == 2) {
        FLOAT const sqrt2_half = SQRT2 * 0.5f;
        
        for (j = BLKSIZE_s - 1; j >= 0; --j) {
            FLOAT const l = wsamp_s[0][sblock][j];
            FLOAT const r = wsamp_s[1][sblock][j];
            wsamp_s[0][sblock][j] = (l + r) * sqrt2_half;
            wsamp_s[1][sblock][j] = (l - r) * sqrt2_half;
        }
    }

    
    fftenergy_s[sblock][0] = (*wsamp_s)[sblock][0];
    fftenergy_s[sblock][0] *= fftenergy_s[sblock][0];
    for (j = BLKSIZE_s / 2 - 1; j >= 0; --j) {
        FLOAT const re = (*wsamp_s)[sblock][BLKSIZE_s / 2 - j];
        FLOAT const im = (*wsamp_s)[sblock][BLKSIZE_s / 2 + j];
        fftenergy_s[sblock][BLKSIZE_s / 2 - j] = (re * re + im * im) * 0.5f;
    }
}


    
static void
vbrpsy_compute_loudness_approximation_l(lame_internal_flags * gfc, int gr_out, int chn,
                                        const FLOAT fftenergy[HBLKSIZE])
{
    PsyStateVar_t *psv = &gfc->sv_psy;
    if (chn < 2) {      
        gfc->ov_psy.loudness_sq[gr_out][chn] = psv->loudness_sq_save[chn];
        psv->loudness_sq_save[chn] = psycho_loudness_approx(fftenergy, gfc->ATH->eql_w);
    }
}


    
static void
vbrpsy_attack_detection(lame_internal_flags * gfc, const sample_t * const buffer[2], int gr_out,
                        III_psy_ratio masking_ratio[2][2], III_psy_ratio masking_MS_ratio[2][2],
                        FLOAT energy[4], FLOAT sub_short_factor[4][3], int ns_attacks[4][4],
                        int uselongblock[2])
{
    FLOAT   ns_hpfsmpl[2][576];
    SessionConfig_t const *const cfg = &gfc->cfg;
    PsyStateVar_t *const psv = &gfc->sv_psy;
    plotting_data *plt = cfg->analysis ? gfc->pinfo : 0;
    int const n_chn_out = cfg->channels_out;
    
    int const n_chn_psy = (cfg->mode == JOINT_STEREO) ? 4 : n_chn_out;
    int     chn, i, j;

    memset(&ns_hpfsmpl[0][0], 0, sizeof(ns_hpfsmpl));
    
    
    for (chn = 0; chn < n_chn_out; chn++) {
        static const FLOAT fircoef[] = {
            -8.65163e-18 * 2, -0.00851586 * 2, -6.74764e-18 * 2, 0.0209036 * 2,
            -3.36639e-17 * 2, -0.0438162 * 2, -1.54175e-17 * 2, 0.0931738 * 2,
            -5.52212e-17 * 2, -0.313819 * 2
        };
        
        const sample_t *const firbuf = &buffer[chn][576 - 350 - NSFIRLEN + 192];
        assert(dimension_of(fircoef) == ((NSFIRLEN - 1) / 2));
        for (i = 0; i < 576; i++) {
            FLOAT   sum1, sum2;
            sum1 = firbuf[i + 10];
            sum2 = 0.0;
            for (j = 0; j < ((NSFIRLEN - 1) / 2) - 1; j += 2) {
                sum1 += fircoef[j] * (firbuf[i + j] + firbuf[i + NSFIRLEN - j]);
                sum2 += fircoef[j + 1] * (firbuf[i + j + 1] + firbuf[i + NSFIRLEN - j - 1]);
            }
            ns_hpfsmpl[chn][i] = sum1 + sum2;
        }
        masking_ratio[gr_out][chn].en = psv->en[chn];
        masking_ratio[gr_out][chn].thm = psv->thm[chn];
        if (n_chn_psy > 2) {
            
            
            masking_MS_ratio[gr_out][chn].en = psv->en[chn + 2];
            masking_MS_ratio[gr_out][chn].thm = psv->thm[chn + 2];
        }
    }
    for (chn = 0; chn < n_chn_psy; chn++) {
        FLOAT   attack_intensity[12];
        FLOAT   en_subshort[12];
        FLOAT   en_short[4] = { 0, 0, 0, 0 };
        FLOAT const *pf = ns_hpfsmpl[chn & 1];
        int     ns_uselongblock = 1;

        if (chn == 2) {
            for (i = 0, j = 576; j > 0; ++i, --j) {
                FLOAT const l = ns_hpfsmpl[0][i];
                FLOAT const r = ns_hpfsmpl[1][i];
                ns_hpfsmpl[0][i] = l + r;
                ns_hpfsmpl[1][i] = l - r;
            }
        }
        
        
        for (i = 0; i < 3; i++) {
            en_subshort[i] = psv->last_en_subshort[chn][i + 6];
            assert(psv->last_en_subshort[chn][i + 4] > 0);
            attack_intensity[i] = en_subshort[i] / psv->last_en_subshort[chn][i + 4];
            en_short[0] += en_subshort[i];
        }

        for (i = 0; i < 9; i++) {
            FLOAT const *const pfe = pf + 576 / 9;
            FLOAT   p = 1.;
            for (; pf < pfe; pf++)
                if (p < fabs(*pf))
                    p = fabs(*pf);
            psv->last_en_subshort[chn][i] = en_subshort[i + 3] = p;
            en_short[1 + i / 3] += p;
            if (p > en_subshort[i + 3 - 2]) {
                assert(en_subshort[i + 3 - 2] > 0);
                p = p / en_subshort[i + 3 - 2];
            }
            else if (en_subshort[i + 3 - 2] > p * 10.0f) {
                assert(p > 0);
                p = en_subshort[i + 3 - 2] / (p * 10.0f);
            }
            else {
                p = 0.0;
            }
            attack_intensity[i + 3] = p;
        }

        
        for (i = 0; i < 3; ++i) {
            FLOAT const enn =
                en_subshort[i * 3 + 3] + en_subshort[i * 3 + 4] + en_subshort[i * 3 + 5];
            FLOAT   factor = 1.f;
            if (en_subshort[i * 3 + 5] * 6 < enn) {
                factor *= 0.5f;
                if (en_subshort[i * 3 + 4] * 6 < enn) {
                    factor *= 0.5f;
                }
            }
            sub_short_factor[chn][i] = factor;
        }

        if (plt) {
            FLOAT   x = attack_intensity[0];
            for (i = 1; i < 12; i++) {
                if (x < attack_intensity[i]) {
                    x = attack_intensity[i];
                }
            }
            plt->ers[gr_out][chn] = plt->ers_save[chn];
            plt->ers_save[chn] = x;
        }

        
        {
            FLOAT   x = gfc->cd_psy->attack_threshold[chn];
            for (i = 0; i < 12; i++) {
                if (ns_attacks[chn][i / 3] == 0) {
                    if (attack_intensity[i] > x) {
                        ns_attacks[chn][i / 3] = (i % 3) + 1;
                    }
                }
            }
        }
        
        
        
        
        for (i = 1; i < 4; i++) {
            FLOAT const u = en_short[i - 1];
            FLOAT const v = en_short[i];
            FLOAT const m = Max(u, v);
            if (m < 40000) { 
                if (u < 1.7f * v && v < 1.7f * u) { 
                    if (i == 1 && ns_attacks[chn][0] <= ns_attacks[chn][i]) {
                        ns_attacks[chn][0] = 0;
                    }
                    ns_attacks[chn][i] = 0;
                }
            }
        }

        if (ns_attacks[chn][0] <= psv->last_attacks[chn]) {
            ns_attacks[chn][0] = 0;
        }

        if (psv->last_attacks[chn] == 3 ||
            ns_attacks[chn][0] + ns_attacks[chn][1] + ns_attacks[chn][2] + ns_attacks[chn][3]) {
            ns_uselongblock = 0;

            if (ns_attacks[chn][1] && ns_attacks[chn][0]) {
                ns_attacks[chn][1] = 0;
            }
            if (ns_attacks[chn][2] && ns_attacks[chn][1]) {
                ns_attacks[chn][2] = 0;
            }
            if (ns_attacks[chn][3] && ns_attacks[chn][2]) {
                ns_attacks[chn][3] = 0;
            }
        }

        if (chn < 2) {
            uselongblock[chn] = ns_uselongblock;
        }
        else {
            if (ns_uselongblock == 0) {
                uselongblock[0] = uselongblock[1] = 0;
            }
        }

        
        energy[chn] = psv->tot_ener[chn];
    }
}


static void
vbrpsy_skip_masking_s(lame_internal_flags * gfc, int chn, int sblock)
{
    if (sblock == 0) {
        FLOAT  *nbs2 = &gfc->sv_psy.nb_s2[chn][0];
        FLOAT  *nbs1 = &gfc->sv_psy.nb_s1[chn][0];
        int const n = gfc->cd_psy->s.npart;
        int     b;
        for (b = 0; b < n; b++) {
            nbs2[b] = nbs1[b];
        }
    }
}


static void
vbrpsy_calc_mask_index_s(lame_internal_flags const *gfc, FLOAT const *max,
                         FLOAT const *avg, unsigned char *mask_idx)
{
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->s;
    FLOAT   m, a;
    int     b, k;
    int const last_tab_entry = dimension_of(tab) - 1;
    b = 0;
    a = avg[b] + avg[b + 1];
    assert(a >= 0);
    if (a > 0.0f) {
        m = max[b];
        if (m < max[b + 1])
            m = max[b + 1];
        assert((gds->numlines[b] + gds->numlines[b + 1] - 1) > 0);
        a = 20.0f * (m * 2.0f - a)
            / (a * (gds->numlines[b] + gds->numlines[b + 1] - 1));
        k = (int) a;
        if (k > last_tab_entry)
            k = last_tab_entry;
        mask_idx[b] = k;
    }
    else {
        mask_idx[b] = 0;
    }

    for (b = 1; b < gds->npart - 1; b++) {
        a = avg[b - 1] + avg[b] + avg[b + 1];
        assert(b + 1 < gds->npart);
        assert(a >= 0);
        if (a > 0.0) {
            m = max[b - 1];
            if (m < max[b])
                m = max[b];
            if (m < max[b + 1])
                m = max[b + 1];
            assert((gds->numlines[b - 1] + gds->numlines[b] + gds->numlines[b + 1] - 1) > 0);
            a = 20.0f * (m * 3.0f - a)
                / (a * (gds->numlines[b - 1] + gds->numlines[b] + gds->numlines[b + 1] - 1));
            k = (int) a;
            if (k > last_tab_entry)
                k = last_tab_entry;
            mask_idx[b] = k;
        }
        else {
            mask_idx[b] = 0;
        }
    }
    assert(b > 0);
    assert(b == gds->npart - 1);

    a = avg[b - 1] + avg[b];
    assert(a >= 0);
    if (a > 0.0f) {
        m = max[b - 1];
        if (m < max[b])
            m = max[b];
        assert((gds->numlines[b - 1] + gds->numlines[b] - 1) > 0);
        a = 20.0f * (m * 2.0f - a)
            / (a * (gds->numlines[b - 1] + gds->numlines[b] - 1));
        k = (int) a;
        if (k > last_tab_entry)
            k = last_tab_entry;
        mask_idx[b] = k;
    }
    else {
        mask_idx[b] = 0;
    }
    assert(b == (gds->npart - 1));
}


static void
vbrpsy_compute_masking_s(lame_internal_flags * gfc, const FLOAT(*fftenergy_s)[HBLKSIZE_s],
                         FLOAT * eb, FLOAT * thr, int chn, int sblock)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->s;
    FLOAT   max[CBANDS], avg[CBANDS];
    int     i, j, b;
    unsigned char mask_idx_s[CBANDS];

    memset(max, 0, sizeof(max));
    memset(avg, 0, sizeof(avg));

    for (b = j = 0; b < gds->npart; ++b) {
        FLOAT   ebb = 0, m = 0;
        int const n = gds->numlines[b];
        for (i = 0; i < n; ++i, ++j) {
            FLOAT const el = fftenergy_s[sblock][j];
            ebb += el;
            if (m < el)
                m = el;
        }
        eb[b] = ebb;
        assert(ebb >= 0);
        max[b] = m;
        assert(n > 0);
        avg[b] = ebb * gds->rnumlines[b];
        assert(avg[b] >= 0);
    }
    assert(b == gds->npart);
    assert(j == 129);
    vbrpsy_calc_mask_index_s(gfc, max, avg, mask_idx_s);
    for (j = b = 0; b < gds->npart; b++) {
        int     kk = gds->s3ind[b][0];
        int const last = gds->s3ind[b][1];
        int const delta = mask_add_delta(mask_idx_s[b]);
        int     dd, dd_n;
        FLOAT   x, ecb, avg_mask;
        FLOAT const masking_lower = gds->masking_lower[b] * gfc->sv_qnt.masking_lower;

        dd = mask_idx_s[kk];
        dd_n = 1;
        ecb = gds->s3[j] * eb[kk] * tab[mask_idx_s[kk]];
        ++j, ++kk;
        while (kk <= last) {
            dd += mask_idx_s[kk];
            dd_n += 1;
            x = gds->s3[j] * eb[kk] * tab[mask_idx_s[kk]];
            ecb = vbrpsy_mask_add(ecb, x, kk - b, delta);
            ++j, ++kk;
        }
        dd = (1 + 2 * dd) / (2 * dd_n);
        avg_mask = tab[dd] * 0.5f;
        ecb *= avg_mask;
#if 0                   
        if (psv->blocktype_old[chn & 0x01] == SHORT_TYPE) {
            
            FLOAT const t1 = rpelev_s * psv->nb_s1[chn][b];
            FLOAT const t2 = rpelev2_s * psv->nb_s2[chn][b];
            FLOAT const tm = (t2 > 0) ? Min(ecb, t2) : ecb;
            thr[b] = (t1 > 0) ? NS_INTERP(Min(tm, t1), ecb, 0.6) : ecb;
        }
        else {
            
            FLOAT const t1 = rpelev_s * psv->nb_s1[chn][b];
            thr[b] = (t1 > 0) ? NS_INTERP(Min(ecb, t1), ecb, 0.6) : ecb;
        }
#else 
        thr[b] = ecb;
#endif
        psv->nb_s2[chn][b] = psv->nb_s1[chn][b];
        psv->nb_s1[chn][b] = ecb;
        {
            
            x = max[b];
            x *= gds->minval[b];
            x *= avg_mask;
            if (thr[b] > x) {
                thr[b] = x;
            }
        }
        if (masking_lower > 1) {
            thr[b] *= masking_lower;
        }
        if (thr[b] > eb[b]) {
            thr[b] = eb[b];
        }
        if (masking_lower < 1) {
            thr[b] *= masking_lower;
        }

        assert(thr[b] >= 0);
    }
    for (; b < CBANDS; ++b) {
        eb[b] = 0;
        thr[b] = 0;
    }
}


static void
vbrpsy_compute_masking_l(lame_internal_flags * gfc, const FLOAT fftenergy[HBLKSIZE],
                         FLOAT eb_l[CBANDS], FLOAT thr[CBANDS], int chn)
{
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gdl = &gfc->cd_psy->l;
    FLOAT   max[CBANDS], avg[CBANDS];
    unsigned char mask_idx_l[CBANDS + 2];
    int     k, b;

 
    calc_energy(gdl, fftenergy, eb_l, max, avg);
    calc_mask_index_l(gfc, max, avg, mask_idx_l);

 
    k = 0;
    for (b = 0; b < gdl->npart; b++) {
        FLOAT   x, ecb, avg_mask, t;
        FLOAT const masking_lower = gdl->masking_lower[b] * gfc->sv_qnt.masking_lower;
        
        int     kk = gdl->s3ind[b][0];
        int const last = gdl->s3ind[b][1];
        int const delta = mask_add_delta(mask_idx_l[b]);
        int     dd = 0, dd_n = 0;

        dd = mask_idx_l[kk];
        dd_n += 1;
        ecb = gdl->s3[k] * eb_l[kk] * tab[mask_idx_l[kk]];
        ++k, ++kk;
        while (kk <= last) {
            dd += mask_idx_l[kk];
            dd_n += 1;
            x = gdl->s3[k] * eb_l[kk] * tab[mask_idx_l[kk]];
            t = vbrpsy_mask_add(ecb, x, kk - b, delta);
#if 0
            ecb += eb_l[kk];
            if (ecb > t) {
                ecb = t;
            }
#else
            ecb = t;
#endif
            ++k, ++kk;
        }
        dd = (1 + 2 * dd) / (2 * dd_n);
        avg_mask = tab[dd] * 0.5f;
        ecb *= avg_mask;

        
        
        
        if (psv->blocktype_old[chn & 0x01] == SHORT_TYPE) {
            FLOAT const ecb_limit = rpelev * psv->nb_l1[chn][b];
            if (ecb_limit > 0) {
                thr[b] = Min(ecb, ecb_limit);
            }
            else {
                
                thr[b] = Min(ecb, eb_l[b] * NS_PREECHO_ATT2);
            }
        }
        else {
            FLOAT   ecb_limit_2 = rpelev2 * psv->nb_l2[chn][b];
            FLOAT   ecb_limit_1 = rpelev * psv->nb_l1[chn][b];
            FLOAT   ecb_limit;
            if (ecb_limit_2 <= 0) {
                ecb_limit_2 = ecb;
            }
            if (ecb_limit_1 <= 0) {
                ecb_limit_1 = ecb;
            }
            if (psv->blocktype_old[chn & 0x01] == NORM_TYPE) {
                ecb_limit = Min(ecb_limit_1, ecb_limit_2);
            }
            else {
                ecb_limit = ecb_limit_1;
            }
            thr[b] = Min(ecb, ecb_limit);
        }
        psv->nb_l2[chn][b] = psv->nb_l1[chn][b];
        psv->nb_l1[chn][b] = ecb;
        {
            
            x = max[b];
            x *= gdl->minval[b];
            x *= avg_mask;
            if (thr[b] > x) {
                thr[b] = x;
            }
        }
        if (masking_lower > 1) {
            thr[b] *= masking_lower;
        }
        if (thr[b] > eb_l[b]) {
            thr[b] = eb_l[b];
        }
        if (masking_lower < 1) {
            thr[b] *= masking_lower;
        }
        assert(thr[b] >= 0);
    }
    for (; b < CBANDS; ++b) {
        eb_l[b] = 0;
        thr[b] = 0;
    }
}


static void
vbrpsy_compute_block_type(SessionConfig_t const *cfg, int *uselongblock)
{
    int     chn;

    if (cfg->short_blocks == short_block_coupled
        
        
        
        && !(uselongblock[0] && uselongblock[1]))
        uselongblock[0] = uselongblock[1] = 0;

    for (chn = 0; chn < cfg->channels_out; chn++) {
        
        if (cfg->short_blocks == short_block_dispensed) {
            uselongblock[chn] = 1;
        }
        if (cfg->short_blocks == short_block_forced) {
            uselongblock[chn] = 0;
        }
    }
}


static void
vbrpsy_apply_block_type(PsyStateVar_t * psv, int nch, int const *uselongblock, int *blocktype_d)
{
    int     chn;

    
    for (chn = 0; chn < nch; chn++) {
        int     blocktype = NORM_TYPE;
        

        if (uselongblock[chn]) {
            
            assert(psv->blocktype_old[chn] != START_TYPE);
            if (psv->blocktype_old[chn] == SHORT_TYPE)
                blocktype = STOP_TYPE;
        }
        else {
            
            blocktype = SHORT_TYPE;
            if (psv->blocktype_old[chn] == NORM_TYPE) {
                psv->blocktype_old[chn] = START_TYPE;
            }
            if (psv->blocktype_old[chn] == STOP_TYPE)
                psv->blocktype_old[chn] = SHORT_TYPE;
        }

        blocktype_d[chn] = psv->blocktype_old[chn]; 
        psv->blocktype_old[chn] = blocktype; 
    }
}




static void
vbrpsy_compute_MS_thresholds(const FLOAT eb[4][CBANDS], FLOAT thr[4][CBANDS],
                             const FLOAT cb_mld[CBANDS], const FLOAT ath_cb[CBANDS], FLOAT athlower,
                             FLOAT msfix, int n)
{
    FLOAT const msfix2 = msfix * 2.f;
    FLOAT   rside, rmid;
    int     b;
    for (b = 0; b < n; ++b) {
        FLOAT const ebM = eb[2][b];
        FLOAT const ebS = eb[3][b];
        FLOAT const thmL = thr[0][b];
        FLOAT const thmR = thr[1][b];
        FLOAT   thmM = thr[2][b];
        FLOAT   thmS = thr[3][b];

        
        
        
        if (thmL <= 1.58f * thmR && thmR <= 1.58f * thmL) {
            FLOAT const mld_m = cb_mld[b] * ebS;
            FLOAT const mld_s = cb_mld[b] * ebM;
            FLOAT const tmp_m = Min(thmS, mld_m);
            FLOAT const tmp_s = Min(thmM, mld_s);
            rmid = Max(thmM, tmp_m);
            rside = Max(thmS, tmp_s);
        }
        else {
            rmid = thmM;
            rside = thmS;
        }
        if (msfix > 0.f) {
            
            
            
            
            FLOAT   thmLR, thmMS;
            FLOAT const ath = ath_cb[b] * athlower;
            FLOAT const tmp_l = Max(thmL, ath);
            FLOAT const tmp_r = Max(thmR, ath);
            thmLR = Min(tmp_l, tmp_r);
            thmM = Max(rmid, ath);
            thmS = Max(rside, ath);
            thmMS = thmM + thmS;
            if (thmMS > 0.f && (thmLR * msfix2) < thmMS) {
                FLOAT const f = thmLR * msfix2 / thmMS;
                thmM *= f;
                thmS *= f;
                assert(thmMS > 0.f);
            }
            rmid = Min(thmM, rmid);
            rside = Min(thmS, rside);
        }
        if (rmid > ebM) {
            rmid = ebM;
        }
        if (rside > ebS) {
            rside = ebS;
        }
        thr[2][b] = rmid;
        thr[3][b] = rside;
    }
}




int
L3psycho_anal_vbr(lame_internal_flags * gfc,
                  const sample_t * const buffer[2], int gr_out,
                  III_psy_ratio masking_ratio[2][2],
                  III_psy_ratio masking_MS_ratio[2][2],
                  FLOAT percep_entropy[2], FLOAT percep_MS_entropy[2],
                  FLOAT energy[4], int blocktype_d[2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_CB2SB_t const *const gdl = &gfc->cd_psy->l;
    PsyConst_CB2SB_t const *const gds = &gfc->cd_psy->s;
    plotting_data *plt = cfg->analysis ? gfc->pinfo : 0;

    III_psy_xmin last_thm[4];

    
    FLOAT(*wsamp_l)[BLKSIZE];
    FLOAT(*wsamp_s)[3][BLKSIZE_s];
    FLOAT   fftenergy[HBLKSIZE];
    FLOAT   fftenergy_s[3][HBLKSIZE_s];
    FLOAT   wsamp_L[2][BLKSIZE];
    FLOAT   wsamp_S[2][3][BLKSIZE_s];
    FLOAT   eb[4][CBANDS], thr[4][CBANDS];

    FLOAT   sub_short_factor[4][3];
    FLOAT   thmm;
    FLOAT const pcfact = 0.6f;
    FLOAT const ath_factor =
        (cfg->msfix > 0.f) ? (cfg->ATH_offset_factor * gfc->ATH->adjust_factor) : 1.f;

    const   FLOAT(*const_eb)[CBANDS] = (const FLOAT(*)[CBANDS]) eb;
    const   FLOAT(*const_fftenergy_s)[HBLKSIZE_s] = (const FLOAT(*)[HBLKSIZE_s]) fftenergy_s;

    
    int     ns_attacks[4][4] = { {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} };
    int     uselongblock[2];

    
    int     chn, sb, sblock;

    
    int const n_chn_psy = (cfg->mode == JOINT_STEREO) ? 4 : cfg->channels_out;

    memcpy(&last_thm[0], &psv->thm[0], sizeof(last_thm));

    vbrpsy_attack_detection(gfc, buffer, gr_out, masking_ratio, masking_MS_ratio, energy,
                            sub_short_factor, ns_attacks, uselongblock);

    vbrpsy_compute_block_type(cfg, uselongblock);

    
    {
        for (chn = 0; chn < n_chn_psy; chn++) {
            int const ch01 = chn & 0x01;

            wsamp_l = wsamp_L + ch01;
            vbrpsy_compute_fft_l(gfc, buffer, chn, gr_out, fftenergy, wsamp_l);
            vbrpsy_compute_loudness_approximation_l(gfc, gr_out, chn, fftenergy);
            vbrpsy_compute_masking_l(gfc, fftenergy, eb[chn], thr[chn], chn);
        }
        if (cfg->mode == JOINT_STEREO) {
            if ((uselongblock[0] + uselongblock[1]) == 2) {
                vbrpsy_compute_MS_thresholds(const_eb, thr, gdl->mld_cb, gfc->ATH->cb_l,
                                             ath_factor, cfg->msfix, gdl->npart);
            }
        }
        
        for (chn = 0; chn < n_chn_psy; chn++) {
            convert_partition2scalefac_l(gfc, eb[chn], thr[chn], chn);
            convert_partition2scalefac_l_to_s(gfc, eb[chn], thr[chn], chn);
        }
    }
    
    {
        int const force_short_block_calc = gfc->cd_psy->force_short_block_calc;
        for (sblock = 0; sblock < 3; sblock++) {
            for (chn = 0; chn < n_chn_psy; ++chn) {
                int const ch01 = chn & 0x01;
                if (uselongblock[ch01] && !force_short_block_calc) {
                    vbrpsy_skip_masking_s(gfc, chn, sblock);
                }
                else {
                    
                    wsamp_s = wsamp_S + ch01;
                    vbrpsy_compute_fft_s(gfc, buffer, chn, sblock, fftenergy_s, wsamp_s);
                    vbrpsy_compute_masking_s(gfc, const_fftenergy_s, eb[chn], thr[chn], chn,
                                             sblock);
                }
            }
            if (cfg->mode == JOINT_STEREO) {
                if ((uselongblock[0] + uselongblock[1]) == 0) {
                    vbrpsy_compute_MS_thresholds(const_eb, thr, gds->mld_cb, gfc->ATH->cb_s,
                                                 ath_factor, cfg->msfix, gds->npart);
                }
            }
            
            for (chn = 0; chn < n_chn_psy; ++chn) {
                int const ch01 = chn & 0x01;
                if (!uselongblock[ch01] || force_short_block_calc) {
                    convert_partition2scalefac_s(gfc, eb[chn], thr[chn], chn, sblock);
                }
            }
        }

        
        for (chn = 0; chn < n_chn_psy; chn++) {
            for (sb = 0; sb < SBMAX_s; sb++) {
                FLOAT   new_thmm[3], prev_thm, t1, t2;
                for (sblock = 0; sblock < 3; sblock++) {
                    thmm = psv->thm[chn].s[sb][sblock];
                    thmm *= NS_PREECHO_ATT0;

                    t1 = t2 = thmm;

                    if (sblock > 0) {
                        prev_thm = new_thmm[sblock - 1];
                    }
                    else {
                        prev_thm = last_thm[chn].s[sb][2];
                    }
                    if (ns_attacks[chn][sblock] >= 2 || ns_attacks[chn][sblock + 1] == 1) {
                        t1 = NS_INTERP(prev_thm, thmm, NS_PREECHO_ATT1 * pcfact);
                    }
                    thmm = Min(t1, thmm);
                    if (ns_attacks[chn][sblock] == 1) {
                        t2 = NS_INTERP(prev_thm, thmm, NS_PREECHO_ATT2 * pcfact);
                    }
                    else if ((sblock == 0 && psv->last_attacks[chn] == 3)
                             || (sblock > 0 && ns_attacks[chn][sblock - 1] == 3)) { 
                        switch (sblock) {
                        case 0:
                            prev_thm = last_thm[chn].s[sb][1];
                            break;
                        case 1:
                            prev_thm = last_thm[chn].s[sb][2];
                            break;
                        case 2:
                            prev_thm = new_thmm[0];
                            break;
                        }
                        t2 = NS_INTERP(prev_thm, thmm, NS_PREECHO_ATT2 * pcfact);
                    }

                    thmm = Min(t1, thmm);
                    thmm = Min(t2, thmm);

                    
                    thmm *= sub_short_factor[chn][sblock];

                    new_thmm[sblock] = thmm;
                }
                for (sblock = 0; sblock < 3; sblock++) {
                    psv->thm[chn].s[sb][sblock] = new_thmm[sblock];
                }
            }
        }
    }
    for (chn = 0; chn < n_chn_psy; chn++) {
        psv->last_attacks[chn] = ns_attacks[chn][2];
    }


    
    vbrpsy_apply_block_type(psv, cfg->channels_out, uselongblock, blocktype_d);

    
    for (chn = 0; chn < n_chn_psy; chn++) {
        FLOAT  *ppe;
        int     type;
        III_psy_ratio const *mr;

        if (chn > 1) {
            ppe = percep_MS_entropy - 2;
            type = NORM_TYPE;
            if (blocktype_d[0] == SHORT_TYPE || blocktype_d[1] == SHORT_TYPE)
                type = SHORT_TYPE;
            mr = &masking_MS_ratio[gr_out][chn - 2];
        }
        else {
            ppe = percep_entropy;
            type = blocktype_d[chn];
            mr = &masking_ratio[gr_out][chn];
        }
        if (type == SHORT_TYPE) {
            ppe[chn] = pecalc_s(mr, gfc->sv_qnt.masking_lower);
        }
        else {
            ppe[chn] = pecalc_l(mr, gfc->sv_qnt.masking_lower);
        }

        if (plt) {
            plt->pe[gr_out][chn] = ppe[chn];
        }
    }
    return 0;
}





static  FLOAT
s3_func(FLOAT bark)
{
    FLOAT   tempx, x, tempy, temp;
    tempx = bark;
    if (tempx >= 0)
        tempx *= 3;
    else
        tempx *= 1.5;

    if (tempx >= 0.5 && tempx <= 2.5) {
        temp = tempx - 0.5;
        x = 8.0 * (temp * temp - 2.0 * temp);
    }
    else
        x = 0.0;
    tempx += 0.474;
    tempy = 15.811389 + 7.5 * tempx - 17.5 * sqrt(1.0 + tempx * tempx);

    if (tempy <= -60.0)
        return 0.0;

    tempx = exp((x + tempy) * LN_TO_LOG10);

    
    tempx /= .6609193;
    return tempx;
}

#if 0
static  FLOAT
norm_s3_func(void)
{
    double  lim_a = 0, lim_b = 0;
    double  x = 0, l, h;
    for (x = 0; s3_func(x) > 1e-20; x -= 1);
    l = x;
    h = 0;
    while (fabs(h - l) > 1e-12) {
        x = (h + l) / 2;
        if (s3_func(x) > 0) {
            h = x;
        }
        else {
            l = x;
        }
    }
    lim_a = l;
    for (x = 0; s3_func(x) > 1e-20; x += 1);
    l = 0;
    h = x;
    while (fabs(h - l) > 1e-12) {
        x = (h + l) / 2;
        if (s3_func(x) > 0) {
            l = x;
        }
        else {
            h = x;
        }
    }
    lim_b = h;
    {
        double  sum = 0;
        int const m = 1000;
        int     i;
        for (i = 0; i <= m; ++i) {
            double  x = lim_a + i * (lim_b - lim_a) / m;
            double  y = s3_func(x);
            sum += y;
        }
        {
            double  norm = (m + 1) / (sum * (lim_b - lim_a));
            
            return norm;
        }
    }
}
#endif

static  FLOAT
stereo_demask(double f)
{
    
    
    double  arg = freq2bark(f);
    arg = (Min(arg, 15.5) / 15.5);

    return pow(10.0, 1.25 * (1 - cos(PI * arg)) - 2.5);
}

static void
init_numline(PsyConst_CB2SB_t * gd, FLOAT sfreq, int fft_size,
             int mdct_size, int sbmax, int const *scalepos)
{
    FLOAT   b_frq[CBANDS + 1];
    FLOAT const mdct_freq_frac = sfreq / (2.0f * mdct_size);
    FLOAT const deltafreq = fft_size / (2.0f * mdct_size);
    int     partition[HBLKSIZE] = { 0 };
    int     i, j, ni;
    int     sfb;
    sfreq /= fft_size;
    j = 0;
    ni = 0;
    
    
    for (i = 0; i < CBANDS; i++) {
        FLOAT   bark1;
        int     j2, nl;
        bark1 = freq2bark(sfreq * j);

        b_frq[i] = sfreq * j;

        for (j2 = j; freq2bark(sfreq * j2) - bark1 < DELBARK && j2 <= fft_size / 2; j2++);

        nl = j2 - j;
        gd->numlines[i] = nl;
        gd->rnumlines[i] = (nl > 0) ? (1.0f / nl) : 0;

        ni = i + 1;

        while (j < j2) {
            assert(j < HBLKSIZE);
            partition[j++] = i;
        }
        if (j > fft_size / 2) {
            j = fft_size / 2;
            ++i;
            break;
        }
    }
    assert(i < CBANDS);
    b_frq[i] = sfreq * j;

    gd->n_sb = sbmax;
    gd->npart = ni;

    {
        j = 0;
        for (i = 0; i < gd->npart; i++) {
            int const nl = gd->numlines[i];
            FLOAT const freq = sfreq * (j + nl / 2);
            gd->mld_cb[i] = stereo_demask(freq);
            j += nl;
        }
        for (; i < CBANDS; ++i) {
            gd->mld_cb[i] = 1;
        }
    }
    for (sfb = 0; sfb < sbmax; sfb++) {
        int     i1, i2, bo;
        int     start = scalepos[sfb];
        int     end = scalepos[sfb + 1];

        i1 = floor(.5 + deltafreq * (start - .5));
        if (i1 < 0)
            i1 = 0;
        i2 = floor(.5 + deltafreq * (end - .5));

        if (i2 > fft_size / 2)
            i2 = fft_size / 2;

        bo = partition[i2];
        gd->bm[sfb] = (partition[i1] + partition[i2]) / 2;
        gd->bo[sfb] = bo;

        
        {
            FLOAT const f_tmp = mdct_freq_frac * end;
            FLOAT   bo_w = (f_tmp - b_frq[bo]) / (b_frq[bo + 1] - b_frq[bo]);
            if (bo_w < 0) {
                bo_w = 0;
            }
            else {
                if (bo_w > 1) {
                    bo_w = 1;
                }
            }
            gd->bo_weight[sfb] = bo_w;
        }
        gd->mld[sfb] = stereo_demask(mdct_freq_frac * start);
    }
}

static void
compute_bark_values(PsyConst_CB2SB_t const *gd, FLOAT sfreq, int fft_size,
                    FLOAT * bval, FLOAT * bval_width)
{
    
    int     k, j = 0, ni = gd->npart;
    sfreq /= fft_size;
    for (k = 0; k < ni; k++) {
        int const w = gd->numlines[k];
        FLOAT   bark1, bark2;

        bark1 = freq2bark(sfreq * (j));
        bark2 = freq2bark(sfreq * (j + w - 1));
        bval[k] = .5 * (bark1 + bark2);

        bark1 = freq2bark(sfreq * (j - .5));
        bark2 = freq2bark(sfreq * (j + w - .5));
        bval_width[k] = bark2 - bark1;
        j += w;
    }
}

static int
init_s3_values(FLOAT ** p, int (*s3ind)[2], int npart,
               FLOAT const *bval, FLOAT const *bval_width, FLOAT const *norm)
{
    FLOAT   s3[CBANDS][CBANDS];
    
    int     i, j, k;
    int     numberOfNoneZero = 0;

    memset(&s3[0][0], 0, sizeof(s3));

    
    for (i = 0; i < npart; i++) {
        for (j = 0; j < npart; j++) {
            FLOAT   v = s3_func(bval[i] - bval[j]) * bval_width[j];
            s3[i][j] = v * norm[i];
        }
    }
    for (i = 0; i < npart; i++) {
        for (j = 0; j < npart; j++) {
            if (s3[i][j] > 0.0f)
                break;
        }
        s3ind[i][0] = j;

        for (j = npart - 1; j > 0; j--) {
            if (s3[i][j] > 0.0f)
                break;
        }
        s3ind[i][1] = j;
        numberOfNoneZero += (s3ind[i][1] - s3ind[i][0] + 1);
    }
    *p = malloc(sizeof(FLOAT) * numberOfNoneZero);
    if (!*p)
        return -1;

    k = 0;
    for (i = 0; i < npart; i++)
        for (j = s3ind[i][0]; j <= s3ind[i][1]; j++)
            (*p)[k++] = s3[i][j];

    return 0;
}

int
psymodel_init(lame_global_flags const *gfp)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    SessionConfig_t *const cfg = &gfc->cfg;
    PsyStateVar_t *const psv = &gfc->sv_psy;
    PsyConst_t *gd;
    int     i, j, b, sb, k;
    FLOAT   bvl_a = 13, bvl_b = 24;
    FLOAT   snr_l_a = 0, snr_l_b = 0;
    FLOAT   snr_s_a = -8.25, snr_s_b = -4.5;

    FLOAT   bval[CBANDS];
    FLOAT   bval_width[CBANDS];
    FLOAT   norm[CBANDS];
    FLOAT const sfreq = cfg->samplerate_out;

    FLOAT   xav = 10, xbv = 12;
    FLOAT const minval_low = (0.f - cfg->minval);

    if (gfc->cd_psy != 0) {
        return 0;
    }
    memset(norm, 0, sizeof(norm));

    gd = calloc(1, sizeof(PsyConst_t));
    gfc->cd_psy = gd;

    gd->force_short_block_calc = gfp->experimentalZ;

    psv->blocktype_old[0] = psv->blocktype_old[1] = NORM_TYPE; 

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < CBANDS; ++j) {
            psv->nb_l1[i][j] = 1e20;
            psv->nb_l2[i][j] = 1e20;
            psv->nb_s1[i][j] = psv->nb_s2[i][j] = 1.0;
        }
        for (sb = 0; sb < SBMAX_l; sb++) {
            psv->en[i].l[sb] = 1e20;
            psv->thm[i].l[sb] = 1e20;
        }
        for (j = 0; j < 3; ++j) {
            for (sb = 0; sb < SBMAX_s; sb++) {
                psv->en[i].s[sb][j] = 1e20;
                psv->thm[i].s[sb][j] = 1e20;
            }
            psv->last_attacks[i] = 0;
        }
        for (j = 0; j < 9; j++)
            psv->last_en_subshort[i][j] = 10.;
    }


    
    psv->loudness_sq_save[0] = psv->loudness_sq_save[1] = 0.0;



    
    
    init_numline(&gd->l, sfreq, BLKSIZE, 576, SBMAX_l, gfc->scalefac_band.l);
    assert(gd->l.npart < CBANDS);
    compute_bark_values(&gd->l, sfreq, BLKSIZE, bval, bval_width);

    
    for (i = 0; i < gd->l.npart; i++) {
        double  snr = snr_l_a;
        if (bval[i] >= bvl_a) {
            snr = snr_l_b * (bval[i] - bvl_a) / (bvl_b - bvl_a)
                + snr_l_a * (bvl_b - bval[i]) / (bvl_b - bvl_a);
        }
        norm[i] = pow(10.0, snr / 10.0);
    }
    i = init_s3_values(&gd->l.s3, gd->l.s3ind, gd->l.npart, bval, bval_width, norm);
    if (i)
        return i;

    
    j = 0;
    for (i = 0; i < gd->l.npart; i++) {
        double  x;

        
        x = FLOAT_MAX;
        for (k = 0; k < gd->l.numlines[i]; k++, j++) {
            FLOAT const freq = sfreq * j / (1000.0 * BLKSIZE);
            FLOAT   level;
            
            level = ATHformula(cfg, freq * 1000) - 20; 
            level = pow(10., 0.1 * level); 
            level *= gd->l.numlines[i];
            if (x > level)
                x = level;
        }
        gfc->ATH->cb_l[i] = x;

        
        
        x = 20.0 * (bval[i] / xav - 1.0);
        if (x > 6) {
            x = 30;
        }
        if (x < minval_low) {
            x = minval_low;
        }
        if (cfg->samplerate_out < 44000) {
            x = 30;
        }
        x -= 8.;
        gd->l.minval[i] = pow(10.0, x / 10.) * gd->l.numlines[i];
    }

    
    init_numline(&gd->s, sfreq, BLKSIZE_s, 192, SBMAX_s, gfc->scalefac_band.s);
    assert(gd->s.npart < CBANDS);
    compute_bark_values(&gd->s, sfreq, BLKSIZE_s, bval, bval_width);

    
    j = 0;
    for (i = 0; i < gd->s.npart; i++) {
        double  x;
        double  snr = snr_s_a;
        if (bval[i] >= bvl_a) {
            snr = snr_s_b * (bval[i] - bvl_a) / (bvl_b - bvl_a)
                + snr_s_a * (bvl_b - bval[i]) / (bvl_b - bvl_a);
        }
        norm[i] = pow(10.0, snr / 10.0);

        
        x = FLOAT_MAX;
        for (k = 0; k < gd->s.numlines[i]; k++, j++) {
            FLOAT const freq = sfreq * j / (1000.0 * BLKSIZE_s);
            FLOAT   level;
            
            level = ATHformula(cfg, freq * 1000) - 20; 
            level = pow(10., 0.1 * level); 
            level *= gd->s.numlines[i];
            if (x > level)
                x = level;
        }
        gfc->ATH->cb_s[i] = x;

        
        x = 7.0 * (bval[i] / xbv - 1.0);
        if (bval[i] > xbv) {
            x *= 1 + log(1 + x) * 3.1;
        }
        if (bval[i] < xbv) {
            x *= 1 + log(1 - x) * 2.3;
        }
        if (x > 6) {
            x = 30;
        }
        if (x < minval_low) {
            x = minval_low;
        }
        if (cfg->samplerate_out < 44000) {
            x = 30;
        }
        x -= 8;
        gd->s.minval[i] = pow(10.0, x / 10) * gd->s.numlines[i];
    }

    i = init_s3_values(&gd->s.s3, gd->s.s3ind, gd->s.npart, bval, bval_width, norm);
    if (i)
        return i;


    init_mask_add_max_values();
    init_fft(gfc);

    
    gd->decay = exp(-1.0 * LOG10 / (temporalmask_sustain_sec * sfreq / 192.0));

    {
        FLOAT   msfix;
        msfix = NS_MSFIX;
        if (cfg->use_safe_joint_stereo)
            msfix = 1.0;
        if (fabs(cfg->msfix) > 0.0)
            msfix = cfg->msfix;
        cfg->msfix = msfix;

        
        for (b = 0; b < gd->l.npart; b++)
            if (gd->l.s3ind[b][1] > gd->l.npart - 1)
                gd->l.s3ind[b][1] = gd->l.npart - 1;
    }

    
#define  frame_duration (576. * cfg->mode_gr / sfreq)
    gfc->ATH->decay = pow(10., -12. / 10. * frame_duration);
    gfc->ATH->adjust_factor = 0.01; 
    gfc->ATH->adjust_limit = 1.0; 
#undef  frame_duration

    assert(gd->l.bo[SBMAX_l - 1] <= gd->l.npart);
    assert(gd->s.bo[SBMAX_s - 1] <= gd->s.npart);

    if (cfg->ATHtype != -1) {
        
        FLOAT   freq;
        FLOAT const freq_inc = (FLOAT) cfg->samplerate_out / (FLOAT) (BLKSIZE);
        FLOAT   eql_balance = 0.0;
        freq = 0.0;
        for (i = 0; i < BLKSIZE / 2; ++i) {
            
            
            freq += freq_inc;
            gfc->ATH->eql_w[i] = 1. / pow(10, ATHformula(cfg, freq) / 10);
            eql_balance += gfc->ATH->eql_w[i];
        }
        eql_balance = 1.0 / eql_balance;
        for (i = BLKSIZE / 2; --i >= 0;) { 
            gfc->ATH->eql_w[i] *= eql_balance;
        }
    }
    {
        for (b = j = 0; b < gd->s.npart; ++b) {
            for (i = 0; i < gd->s.numlines[b]; ++i) {
                ++j;
            }
        }
        assert(j == 129);
        for (b = j = 0; b < gd->l.npart; ++b) {
            for (i = 0; i < gd->l.numlines[b]; ++i) {
                ++j;
            }
        }
        assert(j == 513);
    }
    
    {
        float   x = gfp->attackthre;
        float   y = gfp->attackthre_s;
        if (x < 0) {
            x = NSATTACKTHRE;
        }
        if (y < 0) {
            y = NSATTACKTHRE_S;
        }
        gd->attack_threshold[0] = gd->attack_threshold[1] = gd->attack_threshold[2] = x;
        gd->attack_threshold[3] = y;
    }
    {
        float   sk_s = -10.f, sk_l = -4.7f;
        static float const sk[] =
            { -7.4, -7.4, -7.4, -9.5, -7.4, -6.1, -5.5, -4.7, -4.7, -4.7, -4.7 };
        if (gfp->VBR_q < 4) {
            sk_l = sk_s = sk[0];
        }
        else {
            sk_l = sk_s = sk[gfp->VBR_q] + gfp->VBR_q_frac * (sk[gfp->VBR_q] - sk[gfp->VBR_q + 1]);
        }
        b = 0;
        for (; b < gd->s.npart; b++) {
            float   m = (float) (gd->s.npart - b) / gd->s.npart;
            gd->s.masking_lower[b] = powf(10.f, sk_s * m * 0.1f);
        }
        for (; b < CBANDS; ++b) {
            gd->s.masking_lower[b] = 1.f;
        }
        b = 0;
        for (; b < gd->l.npart; b++) {
            float   m = (float) (gd->l.npart - b) / gd->l.npart;
            gd->l.masking_lower[b] = powf(10.f, sk_l * m * 0.1f);
        }
        for (; b < CBANDS; ++b) {
            gd->l.masking_lower[b] = 1.f;
        }
    }
    memcpy(&gd->l_to_s, &gd->l, sizeof(gd->l_to_s));
    init_numline(&gd->l_to_s, sfreq, BLKSIZE, 192, SBMAX_s, gfc->scalefac_band.s);
    return 0;
}
