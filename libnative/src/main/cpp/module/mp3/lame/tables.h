

#ifndef LAME_TABLES_H
#define LAME_TABLES_H

#if 0
typedef struct {
    unsigned char no;
    unsigned char width;
    unsigned char minval_2;
    float   quiet_thr;
    float   norm;
    float   bark;
} type1_t;

typedef struct {
    unsigned char no;
    unsigned char width;
    float   quiet_thr;
    float   norm;
    float   SNR;
    float   bark;
} type2_t;

typedef struct {
    unsigned int no:5;
    unsigned int cbw:3;
    unsigned int bu:6;
    unsigned int bo:6;
    unsigned int w1_576:10;
    unsigned int w2_576:10;
} type34_t;

typedef struct {
    size_t  len1;
    const type1_t *const tab1;
    size_t  len2;
    const type2_t *const tab2;
    size_t  len3;
    const type34_t *const tab3;
    size_t  len4;
    const type34_t *const tab4;
} type5_t;

extern const type5_t table5[6];

#endif

#define HTN	34

struct huffcodetab {
    const unsigned int xlen;          
    const unsigned int linmax;        
    const uint16_t *table;      
    const uint8_t *hlen;        
};

extern const struct huffcodetab ht[HTN];
    
    
    
    

extern const uint8_t t32l[];
extern const uint8_t t33l[];

extern const uint32_t largetbl[16 * 16];
extern const uint32_t table23[3 * 3];
extern const uint32_t table56[4 * 4];

extern const int scfsi_band[5];

extern const int bitrate_table    [3][16];
extern const int samplerate_table [3][ 4];

#endif 
