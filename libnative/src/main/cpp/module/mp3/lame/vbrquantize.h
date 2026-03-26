

#ifndef LAME_VBRQUANTIZE_H
#define LAME_VBRQUANTIZE_H

int     VBR_encode_frame(lame_internal_flags * gfc, const FLOAT xr34orig[2][2][576],
                         const FLOAT l3_xmin[2][2][SFBMAX], const int maxbits[2][2]);

#endif 
