



#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "reservoir.h"

#include "bitstream.h"
#include "lame-analysis.h"
#include "lame_global_flags.h"






int
ResvFrameBegin(lame_internal_flags * gfc, int *mean_bits)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     fullFrameBits;
    int     resvLimit;
    int     maxmp3buf;
    III_side_info_t *const l3_side = &gfc->l3_side;
    int     frameLength;
    int     meanBits;

    frameLength = getframebits(gfc);
    meanBits = (frameLength - cfg->sideinfo_len * 8) / cfg->mode_gr;



    
    resvLimit = (8 * 256) * cfg->mode_gr - 8;

    
    maxmp3buf = cfg->buffer_constraint;
    esv->ResvMax = maxmp3buf - frameLength;
    if (esv->ResvMax > resvLimit)
        esv->ResvMax = resvLimit;
    if (esv->ResvMax < 0 || cfg->disable_reservoir)
        esv->ResvMax = 0;
    
    fullFrameBits = meanBits * cfg->mode_gr + Min(esv->ResvSize, esv->ResvMax);

    if (fullFrameBits > maxmp3buf)
        fullFrameBits = maxmp3buf;

    assert(0 == esv->ResvMax % 8);
    assert(esv->ResvMax >= 0);

    l3_side->resvDrain_pre = 0;

    if (gfc->pinfo != NULL) {
        gfc->pinfo->mean_bits = meanBits / 2; 
        gfc->pinfo->resvsize = esv->ResvSize;
    }
    *mean_bits = meanBits;
    return fullFrameBits;
}



void
ResvMaxBits(lame_internal_flags * gfc, int mean_bits, int *targ_bits, int *extra_bits, int cbr)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    int     add_bits, targBits, extraBits;
    int     ResvSize = esv->ResvSize, ResvMax = esv->ResvMax;

    
    if (cbr)
        ResvSize += mean_bits;

    if (gfc->sv_qnt.substep_shaping & 1)
        ResvMax *= 0.9;

    targBits = mean_bits;

    
    if (ResvSize * 10 > ResvMax * 9) {
        add_bits = ResvSize - (ResvMax * 9) / 10;
        targBits += add_bits;
        gfc->sv_qnt.substep_shaping |= 0x80;
    }
    else {
        add_bits = 0;
        gfc->sv_qnt.substep_shaping &= 0x7f;
        
        
        if (!cfg->disable_reservoir && !(gfc->sv_qnt.substep_shaping & 1))
            targBits -= .1 * mean_bits;
    }


    
    extraBits = (ResvSize < (esv->ResvMax * 6) / 10 ? ResvSize : (esv->ResvMax * 6) / 10);
    extraBits -= add_bits;

    if (extraBits < 0)
        extraBits = 0;

    *targ_bits = targBits;
    *extra_bits = extraBits;
}


void
ResvAdjust(lame_internal_flags * gfc, gr_info const *gi)
{
    gfc->sv_enc.ResvSize -= gi->part2_3_length + gi->part2_length;
}



void
ResvFrameEnd(lame_internal_flags * gfc, int mean_bits)
{
    SessionConfig_t const *const cfg = &gfc->cfg;
    EncStateVar_t *const esv = &gfc->sv_enc;
    III_side_info_t *const l3_side = &gfc->l3_side;
    int     stuffingBits;
    int     over_bits;

    esv->ResvSize += mean_bits * cfg->mode_gr;
    stuffingBits = 0;
    l3_side->resvDrain_post = 0;
    l3_side->resvDrain_pre = 0;

    
    if ((over_bits = esv->ResvSize % 8) != 0)
        stuffingBits += over_bits;


    over_bits = (esv->ResvSize - stuffingBits) - esv->ResvMax;
    if (over_bits > 0) {
        assert(0 == over_bits % 8);
        assert(over_bits >= 0);
        stuffingBits += over_bits;
    }


    
    
    {
        int     mdb_bytes = Min(l3_side->main_data_begin * 8, stuffingBits) / 8;
        l3_side->resvDrain_pre += 8 * mdb_bytes;
        stuffingBits -= 8 * mdb_bytes;
        esv->ResvSize -= 8 * mdb_bytes;
        l3_side->main_data_begin -= mdb_bytes;
    }
    
    l3_side->resvDrain_post += stuffingBits;
    esv->ResvSize -= stuffingBits;
}
