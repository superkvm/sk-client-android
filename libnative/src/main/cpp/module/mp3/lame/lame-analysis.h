

#ifndef LAME_GTKANAL_H
#define LAME_GTKANAL_H


#define READ_AHEAD 40   
#define MAXMPGLAG READ_AHEAD 
#define NUMBACK 6       
#define NUMPINFO (NUMBACK+READ_AHEAD+1)



struct plotting_data {
    int     frameNum;        
    int     frameNum123;
    int     num_samples;     
    double  frametime;       
    double  pcmdata[2][1600];
    double  pcmdata2[2][1152 + 1152 - DECDELAY];
    double  xr[2][2][576];
    double  mpg123xr[2][2][576];
    double  ms_ratio[2];
    double  ms_ener_ratio[2];

    
    double  energy_save[4][BLKSIZE]; 
    double  energy[2][4][BLKSIZE];
    double  pe[2][4];
    double  thr[2][4][SBMAX_l];
    double  en[2][4][SBMAX_l];
    double  thr_s[2][4][3 * SBMAX_s];
    double  en_s[2][4][3 * SBMAX_s];
    double  ers_save[4];     
    double  ers[2][4];

    double  sfb[2][2][SBMAX_l];
    double  sfb_s[2][2][3 * SBMAX_s];
    double  LAMEsfb[2][2][SBMAX_l];
    double  LAMEsfb_s[2][2][3 * SBMAX_s];

    int     LAMEqss[2][2];
    int     qss[2][2];
    int     big_values[2][2];
    int     sub_gain[2][2][3];

    double  xfsf[2][2][SBMAX_l];
    double  xfsf_s[2][2][3 * SBMAX_s];

    int     over[2][2];
    double  tot_noise[2][2];
    double  max_noise[2][2];
    double  over_noise[2][2];
    int     over_SSD[2][2];
    int     blocktype[2][2];
    int     scalefac_scale[2][2];
    int     preflag[2][2];
    int     mpg123blocktype[2][2];
    int     mixed[2][2];
    int     mainbits[2][2];
    int     sfbits[2][2];
    int     LAMEmainbits[2][2];
    int     LAMEsfbits[2][2];
    int     framesize, stereo, js, ms_stereo, i_stereo, emph, bitrate, sampfreq, maindata;
    int     crc, padding;
    int     scfsi[2], mean_bits, resvsize;
    int     totbits;
};
#ifndef plotting_data_defined
#define plotting_data_defined
typedef struct plotting_data plotting_data;
#endif
#if 0
extern plotting_data *pinfo;
#endif
#endif
