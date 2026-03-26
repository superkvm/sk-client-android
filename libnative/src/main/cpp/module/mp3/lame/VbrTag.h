

#ifndef LAME_VRBTAG_H
#define LAME_VRBTAG_H





#define FRAMES_FLAG     0x0001
#define BYTES_FLAG      0x0002
#define TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

#define NUMTOCENTRIES 100

#ifndef lame_internal_flags_defined
#define lame_internal_flags_defined
struct lame_internal_flags;
typedef struct lame_internal_flags lame_internal_flags;
#endif




typedef struct {
    int     h_id;            
    int     samprate;        
    int     flags;           
    int     frames;          
    int     bytes;           
    int     vbr_scale;       
    unsigned char toc[NUMTOCENTRIES]; 
    int     headersize;      
    int     enc_delay;       
    int     enc_padding;     
} VBRTAGDATA;

int     GetVbrTag(VBRTAGDATA * pTagData, const unsigned char *buf);

int     InitVbrTag(lame_global_flags * gfp);
int     PutVbrTag(lame_global_flags const *gfp, FILE * fid);
void    AddVbrFrame(lame_internal_flags * gfc);
void    UpdateMusicCRC(uint16_t * crc, const unsigned char *buffer, int size);

#endif
