

#ifndef LAME_RESERVOIR_H
#define LAME_RESERVOIR_H

int     ResvFrameBegin(lame_internal_flags * gfc, int *mean_bits);
void    ResvMaxBits(lame_internal_flags * gfc, int mean_bits, int *targ_bits, int *max_bits,
                    int cbr);
void    ResvAdjust(lame_internal_flags * gfc, gr_info const *gi);
void    ResvFrameEnd(lame_internal_flags * gfc, int mean_bits);

#endif 
