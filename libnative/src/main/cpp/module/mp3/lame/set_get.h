

#ifndef __SET_GET_H__
#define __SET_GET_H__

#include "lame.h"

#if defined(__cplusplus)
extern  "C" {
#endif




    int CDECL lame_set_short_threshold(lame_global_flags *, float, float);
    int CDECL lame_set_short_threshold_lrm(lame_global_flags *, float);
    float CDECL lame_get_short_threshold_lrm(const lame_global_flags *);
    int CDECL lame_set_short_threshold_s(lame_global_flags *, float);
    float CDECL lame_get_short_threshold_s(const lame_global_flags *);


    int CDECL lame_set_maskingadjust(lame_global_flags *, float);
    float CDECL lame_get_maskingadjust(const lame_global_flags *);

    int CDECL lame_set_maskingadjust_short(lame_global_flags *, float);
    float CDECL lame_get_maskingadjust_short(const lame_global_flags *);


    int CDECL lame_set_ATHcurve(lame_global_flags *, float);
    float CDECL lame_get_ATHcurve(const lame_global_flags *);

    int CDECL lame_set_preset_notune(lame_global_flags *, int);


    int CDECL lame_set_substep(lame_global_flags *, int);
    int CDECL lame_get_substep(const lame_global_flags *);


    int CDECL lame_set_sfscale(lame_global_flags *, int);
    int CDECL lame_get_sfscale(const lame_global_flags *);


    int CDECL lame_set_subblock_gain(lame_global_flags *, int);
    int CDECL lame_get_subblock_gain(const lame_global_flags *);




    int     apply_preset(lame_global_flags *, int preset, int enforce);

    void CDECL lame_set_tune(lame_t, float); 
    void CDECL lame_set_msfix(lame_t gfp, double msfix);


#if defined(__cplusplus)
}
#endif
#endif
