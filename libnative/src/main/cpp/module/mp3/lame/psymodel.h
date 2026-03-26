

#ifndef LAME_PSYMODEL_H
#define LAME_PSYMODEL_H


int     L3psycho_anal_ns(lame_internal_flags * gfc,
                         const sample_t *const buffer[2], int gr,
                         III_psy_ratio ratio[2][2],
                         III_psy_ratio MS_ratio[2][2],
                         FLOAT pe[2], FLOAT pe_MS[2], FLOAT ener[2], int blocktype_d[2]);

int     L3psycho_anal_vbr(lame_internal_flags * gfc,
                          const sample_t *const buffer[2], int gr,
                          III_psy_ratio ratio[2][2],
                          III_psy_ratio MS_ratio[2][2],
                          FLOAT pe[2], FLOAT pe_MS[2], FLOAT ener[2], int blocktype_d[2]);


int     psymodel_init(lame_global_flags const* gfp);


#define rpelev 2
#define rpelev2 16
#define rpelev_s 2
#define rpelev2_s 16


#define DELBARK .34



#define VO_SCALE (1./( 14752*14752 )/(BLKSIZE/2))

#define temporalmask_sustain_sec 0.01

#define NS_PREECHO_ATT0 0.8
#define NS_PREECHO_ATT1 0.6
#define NS_PREECHO_ATT2 0.3

#define NS_MSFIX 3.5
#define NSATTACKTHRE 4.4
#define NSATTACKTHRE_S 25

#endif 
