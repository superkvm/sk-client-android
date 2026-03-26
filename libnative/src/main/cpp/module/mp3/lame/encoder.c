



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "lame_global_flags.h"
#include "newmdct.h"
#include "psymodel.h"
#include "lame-analysis.h"
#include "bitstream.h"
#include "VbrTag.h"
#include "quantize_pvt.h"




static void
adjust_ATH(lame_internal_flags const *const gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    FLOAT   gr2_max, max_pow;

    if (gfc->ATH->use_adjust == 0) {
        gfc->ATH->adjust_factor = 1.0; 
        return;
    }

    
    
    
    max_pow = gfc->ov_psy.loudness_sq[0][0];
    gr2_max = gfc->ov_psy.loudness_sq[1][0];
    if (cfg->channels_out == 2) {
        max_pow += gfc->ov_psy.loudness_sq[0][1];
        gr2_max += gfc->ov_psy.loudness_sq[1][1];
    }
    else {
        max_pow += max_pow;
        gr2_max += gr2_max;
    }
    if (cfg->mode_gr == 2) {
        max_pow = Max(max_pow, gr2_max);
    }
    max_pow *= 0.5;     

    
    
    max_pow *= gfc->ATH->aa_sensitivity_p;

    

    
    
    
    
    
    
    
    
    
    if (max_pow > 0.03125) { 
        if (gfc->ATH->adjust_factor >= 1.0) {
            gfc->ATH->adjust_factor = 1.0;
        }
        else {
            
            
            
            if (gfc->ATH->adjust_factor < gfc->ATH->adjust_limit) {
                gfc->ATH->adjust_factor = gfc->ATH->adjust_limit;
            }
        }
        gfc->ATH->adjust_limit = 1.0;
    }
    else {              
        
        FLOAT const adj_lim_new = 31.98 * max_pow + 0.000625;
        if (gfc->ATH->adjust_factor >= adj_lim_new) { 
            gfc->ATH->adjust_factor *= adj_lim_new * 0.075 + 0.925;
            if (gfc->ATH->adjust_factor < adj_lim_new) { 
                gfc->ATH->adjust_factor = adj_lim_new;
            }
        }
        else {          
            if (gfc->ATH->adjust_limit >= adj_lim_new) {
                gfc->ATH->adjust_factor = adj_lim_new;
            }
            else {      
                
                if (gfc->ATH->adjust_factor < gfc->ATH->adjust_limit) {
                    gfc->ATH->adjust_factor = gfc->ATH->adjust_limit;
                }
            }
        }
        gfc->ATH->adjust_limit = adj_lim_new;
    }
}



static void
updateStats(lame_internal_flags * const gfc)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncResult_t *eov = &gfc->ov_enc;
    int     gr, ch;
    assert(0 <= eov->bitrate_index && eov->bitrate_index < 16);
    assert(0 <= eov->mode_ext && eov->mode_ext < 4);

    
    eov->bitrate_channelmode_hist[eov->bitrate_index][4]++;
    eov->bitrate_channelmode_hist[15][4]++;

    
    if (cfg->channels_out == 2) {
        eov->bitrate_channelmode_hist[eov->bitrate_index][eov->mode_ext]++;
        eov->bitrate_channelmode_hist[15][eov->mode_ext]++;
    }
    for (gr = 0; gr < cfg->mode_gr; ++gr) {
        for (ch = 0; ch < cfg->channels_out; ++ch) {
            int     bt = gfc->l3_side.tt[gr][ch].block_type;
            if (gfc->l3_side.tt[gr][ch].mixed_block_flag)
                bt = 4;
            eov->bitrate_blocktype_hist[eov->bitrate_index][bt]++;
            eov->bitrate_blocktype_hist[eov->bitrate_index][5]++;
            eov->bitrate_blocktype_hist[15][bt]++;
            eov->bitrate_blocktype_hist[15][5]++;
        }
    }
}




static void
lame_encode_frame_init(lame_internal_flags * gfc, const sample_t *const inbuf[2])
{
    SessionConfig_t const *const cfg = &gfc->cfg;

    int     ch, gr;

    if (gfc->lame_encode_frame_init == 0) {
        sample_t primebuff0[286 + 1152 + 576];
        sample_t primebuff1[286 + 1152 + 576];
        int const framesize = 576 * cfg->mode_gr;
        
        int     i, j;
        gfc->lame_encode_frame_init = 1;
        memset(primebuff0, 0, sizeof(primebuff0));
        memset(primebuff1, 0, sizeof(primebuff1));
        for (i = 0, j = 0; i < 286 + 576 * (1 + cfg->mode_gr); ++i) {
            if (i < framesize) {
                primebuff0[i] = 0;
                if (cfg->channels_out == 2)
                    primebuff1[i] = 0;
            }
            else {
                primebuff0[i] = inbuf[0][j];
                if (cfg->channels_out == 2)
                    primebuff1[i] = inbuf[1][j];
                ++j;
            }
        }
        
        for (gr = 0; gr < cfg->mode_gr; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                gfc->l3_side.tt[gr][ch].block_type = SHORT_TYPE;
            }
        }
        mdct_sub48(gfc, primebuff0, primebuff1);

        
#if 576 < FFTOFFSET
# error FFTOFFSET greater than 576: FFT uses a negative offset
#endif
        
        assert(gfc->sv_enc.mf_size >= (BLKSIZE + framesize - FFTOFFSET));
        
        assert(gfc->sv_enc.mf_size >= (512 + framesize - 32));
    }

}









typedef FLOAT chgrdata[2][2];


int
lame_encode_mp3_frame(       
                         lame_internal_flags * gfc, 
                         sample_t const *inbuf_l, 
                         sample_t const *inbuf_r, 
                         unsigned char *mp3buf, 
                         int mp3buf_size)
{                       
    SessionConfig_t const *const cfg = &gfc->cfg;
    int     mp3count;
    III_psy_ratio masking_LR[2][2]; 
    III_psy_ratio masking_MS[2][2]; 
    const III_psy_ratio (*masking)[2]; 
    const sample_t *inbuf[2];

    FLOAT   tot_ener[2][4];
    FLOAT   ms_ener_ratio[2] = { .5, .5 };
    FLOAT   pe[2][2] = { {0., 0.}, {0., 0.} }, pe_MS[2][2] = { {
    0., 0.}, {
    0., 0.}};
    FLOAT (*pe_use)[2];

    int     ch, gr;

    inbuf[0] = inbuf_l;
    inbuf[1] = inbuf_r;

    if (gfc->lame_encode_frame_init == 0) {
        
        lame_encode_frame_init(gfc, inbuf);

    }


    
    
    gfc->ov_enc.padding = FALSE;
    if ((gfc->sv_enc.slot_lag -= gfc->sv_enc.frac_SpF) < 0) {
        gfc->sv_enc.slot_lag += cfg->samplerate_out;
        gfc->ov_enc.padding = TRUE;
    }



    

    {
        
        int     ret;
        const sample_t *bufp[2] = {0, 0}; 
        int     blocktype[2];

        for (gr = 0; gr < cfg->mode_gr; gr++) {

            for (ch = 0; ch < cfg->channels_out; ch++) {
                bufp[ch] = &inbuf[ch][576 + gr * 576 - FFTOFFSET];
            }
            ret = L3psycho_anal_vbr(gfc, bufp, gr,
                                    masking_LR, masking_MS,
                                    pe[gr], pe_MS[gr], tot_ener[gr], blocktype);
            if (ret != 0)
                return -4;

            if (cfg->mode == JOINT_STEREO) {
                ms_ener_ratio[gr] = tot_ener[gr][2] + tot_ener[gr][3];
                if (ms_ener_ratio[gr] > 0)
                    ms_ener_ratio[gr] = tot_ener[gr][3] / ms_ener_ratio[gr];
            }

            
            for (ch = 0; ch < cfg->channels_out; ch++) {
                gr_info *const cod_info = &gfc->l3_side.tt[gr][ch];
                cod_info->block_type = blocktype[ch];
                cod_info->mixed_block_flag = 0;
            }
        }
    }


    
    adjust_ATH(gfc);


    

    
    mdct_sub48(gfc, inbuf[0], inbuf[1]);


    

    
    gfc->ov_enc.mode_ext = MPG_MD_LR_LR;

    if (cfg->force_ms) {
        gfc->ov_enc.mode_ext = MPG_MD_MS_LR;
    }
    else if (cfg->mode == JOINT_STEREO) {
        

        

        FLOAT   sum_pe_MS = 0;
        FLOAT   sum_pe_LR = 0;
        for (gr = 0; gr < cfg->mode_gr; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                sum_pe_MS += pe_MS[gr][ch];
                sum_pe_LR += pe[gr][ch];
            }
        }

        
        if (sum_pe_MS <= 1.00 * sum_pe_LR) {

            gr_info const *const gi0 = &gfc->l3_side.tt[0][0];
            gr_info const *const gi1 = &gfc->l3_side.tt[cfg->mode_gr - 1][0];

            if (gi0[0].block_type == gi0[1].block_type && gi1[0].block_type == gi1[1].block_type) {

                gfc->ov_enc.mode_ext = MPG_MD_MS_LR;
            }
        }
    }

    
    if (gfc->ov_enc.mode_ext == MPG_MD_MS_LR) {
        masking = (const III_psy_ratio (*)[2])masking_MS; 
        pe_use = pe_MS;
    }
    else {
        masking = (const III_psy_ratio (*)[2])masking_LR; 
        pe_use = pe;
    }


    
    if (cfg->analysis && gfc->pinfo != NULL) {
        for (gr = 0; gr < cfg->mode_gr; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                gfc->pinfo->ms_ratio[gr] = 0;
                gfc->pinfo->ms_ener_ratio[gr] = ms_ener_ratio[gr];
                gfc->pinfo->blocktype[gr][ch] = gfc->l3_side.tt[gr][ch].block_type;
                gfc->pinfo->pe[gr][ch] = pe_use[gr][ch];
                memcpy(gfc->pinfo->xr[gr][ch], &gfc->l3_side.tt[gr][ch].xr[0], sizeof(FLOAT) * 576);
                
                if (gfc->ov_enc.mode_ext == MPG_MD_MS_LR) {
                    gfc->pinfo->ers[gr][ch] = gfc->pinfo->ers[gr][ch + 2];
                    memcpy(gfc->pinfo->energy[gr][ch], gfc->pinfo->energy[gr][ch + 2],
                           sizeof(gfc->pinfo->energy[gr][ch]));
                }
            }
        }
    }


    

    if (cfg->vbr == vbr_off || cfg->vbr == vbr_abr) {
        static FLOAT const fircoef[9] = {
            -0.0207887 * 5, -0.0378413 * 5, -0.0432472 * 5, -0.031183 * 5,
            7.79609e-18 * 5, 0.0467745 * 5, 0.10091 * 5, 0.151365 * 5,
            0.187098 * 5
        };

        int     i;
        FLOAT   f;

        for (i = 0; i < 18; i++)
            gfc->sv_enc.pefirbuf[i] = gfc->sv_enc.pefirbuf[i + 1];

        f = 0.0;
        for (gr = 0; gr < cfg->mode_gr; gr++)
            for (ch = 0; ch < cfg->channels_out; ch++)
                f += pe_use[gr][ch];
        gfc->sv_enc.pefirbuf[18] = f;

        f = gfc->sv_enc.pefirbuf[9];
        for (i = 0; i < 9; i++)
            f += (gfc->sv_enc.pefirbuf[i] + gfc->sv_enc.pefirbuf[18 - i]) * fircoef[i];

        f = (670 * 5 * cfg->mode_gr * cfg->channels_out) / f;
        for (gr = 0; gr < cfg->mode_gr; gr++) {
            for (ch = 0; ch < cfg->channels_out; ch++) {
                pe_use[gr][ch] *= f;
            }
        }
    }
    gfc->iteration_loop(gfc, (const FLOAT (*)[2])pe_use, ms_ener_ratio, masking);


    


    
    (void) format_bitstream(gfc);

    
    mp3count = copy_buffer(gfc, mp3buf, mp3buf_size, 1);


    if (cfg->write_lame_tag) {
        AddVbrFrame(gfc);
    }

    if (cfg->analysis && gfc->pinfo != NULL) {
        int     framesize = 576 * cfg->mode_gr;
        for (ch = 0; ch < cfg->channels_out; ch++) {
            int     j;
            for (j = 0; j < FFTOFFSET; j++)
                gfc->pinfo->pcmdata[ch][j] = gfc->pinfo->pcmdata[ch][j + framesize];
            for (j = FFTOFFSET; j < 1600; j++) {
                gfc->pinfo->pcmdata[ch][j] = inbuf[ch][j - FFTOFFSET];
            }
        }
        gfc->sv_qnt.masking_lower = 1.0;

        set_frame_pinfo(gfc, masking);
    }

    ++gfc->ov_enc.frame_number;

    updateStats(gfc);

    return mp3count;
}
