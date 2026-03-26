

#ifndef GAIN_ANALYSIS_H
#define GAIN_ANALYSIS_H

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#else
# ifdef HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#ifdef __cplusplus
extern  "C" {
#endif


    typedef sample_t Float_t; 


#define PINK_REF                64.82        


#define YULE_ORDER         10
#define BUTTER_ORDER        2
#define YULE_FILTER     filterYule
#define BUTTER_FILTER   filterButter
#define RMS_PERCENTILE      0.95 
#define MAX_SAMP_FREQ   48000L 
#define RMS_WINDOW_TIME_NUMERATOR    1L
#define RMS_WINDOW_TIME_DENOMINATOR 20L 
#define STEPS_per_dB      100 
#define MAX_dB            120 

    enum { GAIN_NOT_ENOUGH_SAMPLES = -24601, GAIN_ANALYSIS_ERROR = 0, GAIN_ANALYSIS_OK =
            1, INIT_GAIN_ANALYSIS_ERROR = 0, INIT_GAIN_ANALYSIS_OK = 1
    };

    enum { MAX_ORDER = (BUTTER_ORDER > YULE_ORDER ? BUTTER_ORDER : YULE_ORDER)
            , MAX_SAMPLES_PER_WINDOW = ((MAX_SAMP_FREQ * RMS_WINDOW_TIME_NUMERATOR) / RMS_WINDOW_TIME_DENOMINATOR + 1) 
    };

    struct replaygain_data {
        Float_t linprebuf[MAX_ORDER * 2];
        Float_t *linpre;     
        Float_t lstepbuf[MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
        Float_t *lstep;      
        Float_t loutbuf[MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
        Float_t *lout;       
        Float_t rinprebuf[MAX_ORDER * 2];
        Float_t *rinpre;     
        Float_t rstepbuf[MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
        Float_t *rstep;
        Float_t routbuf[MAX_SAMPLES_PER_WINDOW + MAX_ORDER];
        Float_t *rout;
        long    sampleWindow; 
        long    totsamp;
        double  lsum;
        double  rsum;
        int     freqindex;
        int     first;
        uint32_t A[STEPS_per_dB * MAX_dB];
        uint32_t B[STEPS_per_dB * MAX_dB];

    };
#ifndef replaygain_data_defined
#define replaygain_data_defined
    typedef struct replaygain_data replaygain_t;
#endif




    int     InitGainAnalysis(replaygain_t * rgData, long samplefreq);
    int     AnalyzeSamples(replaygain_t * rgData, const Float_t * left_samples,
                           const Float_t * right_samples, size_t num_samples, int num_channels);
    Float_t GetTitleGain(replaygain_t * rgData);


#ifdef __cplusplus
}
#endif
#endif                       
