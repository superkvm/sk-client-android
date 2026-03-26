#ifndef LAME_GLOBAL_FLAGS_H
#define LAME_GLOBAL_FLAGS_H

#ifndef lame_internal_flags_defined
#define lame_internal_flags_defined
struct lame_internal_flags;
typedef struct lame_internal_flags lame_internal_flags;
#endif


typedef enum short_block_e {
    short_block_not_set = -1, 
    short_block_allowed = 0, 
    short_block_coupled, 
    short_block_dispensed, 
    short_block_forced  
} short_block_t;


struct lame_global_struct {
    unsigned int class_id;

    
    unsigned long num_samples; 
    int     num_channels;    
    int     samplerate_in;   
    int     samplerate_out;  
    float   scale;           
    float   scale_left;      
    float   scale_right;     

    
    int     analysis;        
    int     write_lame_tag;  
    int     decode_only;     
    int     quality;         
    MPEG_mode mode;          
    int     force_ms;        
    int     free_format;     
    int     findReplayGain;  
    int     decode_on_the_fly; 
    int     write_id3tag_automatic; 

    int     nogap_total;
    int     nogap_current;

    int     substep_shaping;
    int     noise_shaping;
    int     subblock_gain;   
    int     use_best_huffman; 

    
    int     brate;           
    float   compression_ratio; 


    
    int     copyright;       
    int     original;        
    int     extension;       
    int     emphasis;        
    int     error_protection; 
    int     strict_ISO;      

    int     disable_reservoir; 

    
    int     quant_comp;
    int     quant_comp_short;
    int     experimentalY;
    int     experimentalZ;
    int     exp_nspsytune;

    int     preset;

    
    vbr_mode VBR;
    float   VBR_q_frac;      
    int     VBR_q;           
    int     VBR_mean_bitrate_kbps;
    int     VBR_min_bitrate_kbps;
    int     VBR_max_bitrate_kbps;
    int     VBR_hard_min;    


    
    int     lowpassfreq;     
    int     highpassfreq;    
    int     lowpasswidth;    
    int     highpasswidth;   



    
    float   maskingadjust;
    float   maskingadjust_short;
    int     ATHonly;         
    int     ATHshort;        
    int     noATH;           
    int     ATHtype;         
    float   ATHcurve;        
    float   ATH_lower_db;    
    int     athaa_type;      
    float   athaa_sensitivity; 
    short_block_t short_blocks;
    int     useTemporal;     
    float   interChRatio;
    float   msfix;           

    int     tune;            
    float   tune_value_a;    

    float   attackthre;      
    float   attackthre_s;    


    struct {
        void    (*msgf) (const char *format, va_list ap);
        void    (*debugf) (const char *format, va_list ap);
        void    (*errorf) (const char *format, va_list ap);
    } report;

  
    
    
  

    int     lame_allocated_gfp; 



  
    
  
    lame_internal_flags *internal_flags;


    struct {
        int     mmx;
        int     amd3dnow;
        int     sse;

    } asm_optimizations;
};

int     is_lame_global_flags_valid(const lame_global_flags * gfp);

#endif 
