

#ifndef LAME_QUANTIZE_H
#define LAME_QUANTIZE_H

void    CBR_iteration_loop(lame_internal_flags * gfc, const FLOAT pe[2][2],
                           const FLOAT ms_ratio[2], const III_psy_ratio ratio[2][2]);

void    VBR_old_iteration_loop(lame_internal_flags * gfc, const FLOAT pe[2][2],
                               const FLOAT ms_ratio[2], const III_psy_ratio ratio[2][2]);

void    VBR_new_iteration_loop(lame_internal_flags * gfc, const FLOAT pe[2][2],
                               const FLOAT ms_ratio[2], const III_psy_ratio ratio[2][2]);

void    ABR_iteration_loop(lame_internal_flags * gfc, const FLOAT pe[2][2],
                           const FLOAT ms_ratio[2], const III_psy_ratio ratio[2][2]);


#endif 
