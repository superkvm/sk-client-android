




#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "lame.h"
#include "machine.h"

#include "version.h"    







const char *
get_lame_version(void)
{                       
    

#if   LAME_ALPHA_VERSION
    static  const char *const str =
        STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) " "
        "(alpha " STR(LAME_PATCH_VERSION) ", " __DATE__ " " __TIME__ ")";
#elif LAME_BETA_VERSION
    static  const char *const str =
        STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) " "
        "(beta " STR(LAME_PATCH_VERSION) ", " __DATE__ ")";
#elif LAME_RELEASE_VERSION && (LAME_PATCH_VERSION > 0)
    static  const char *const str =
        STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) "." STR(LAME_PATCH_VERSION);
#else
    static  const char *const str =
        STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION);
#endif

    return str;
}




const char *
get_lame_short_version(void)
{
    

#if   LAME_ALPHA_VERSION
    static  const char *const str =
        STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) " (alpha " STR(LAME_PATCH_VERSION) ")";
#elif LAME_BETA_VERSION
    static  const char *const str =
        STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) " (beta " STR(LAME_PATCH_VERSION) ")";
#elif LAME_RELEASE_VERSION && (LAME_PATCH_VERSION > 0)
    static  const char *const str =
        STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) "." STR(LAME_PATCH_VERSION);
#else
    static  const char *const str =
        STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION);
#endif

    return str;
}



const char *
get_lame_very_short_version(void)
{
    
#if   LAME_ALPHA_VERSION
#define P "a"
#elif LAME_BETA_VERSION
#define P "b"
#elif LAME_RELEASE_VERSION && (LAME_PATCH_VERSION > 0)
#define P "r"
#else
#define P ""
#endif
    static  const char *const str =
#if (LAME_PATCH_VERSION > 0)
      "LAME" STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) P STR(LAME_PATCH_VERSION)
#else
      "LAME" STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) P
#endif
      ;
    return str;
}



const char*
get_lame_tag_encoder_short_version(void)
{
    static  const char *const str =
            
    "LAME" STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) P
    ;
    return str;
}



const char *
get_psy_version(void)
{
#if   PSY_ALPHA_VERSION > 0
    static  const char *const str =
        STR(PSY_MAJOR_VERSION) "." STR(PSY_MINOR_VERSION)
        " (alpha " STR(PSY_ALPHA_VERSION) ", " __DATE__ " " __TIME__ ")";
#elif PSY_BETA_VERSION > 0
    static  const char *const str =
        STR(PSY_MAJOR_VERSION) "." STR(PSY_MINOR_VERSION)
        " (beta " STR(PSY_BETA_VERSION) ", " __DATE__ ")";
#else
    static  const char *const str =
        STR(PSY_MAJOR_VERSION) "." STR(PSY_MINOR_VERSION);
#endif

    return str;
}




const char *
get_lame_url(void)
{
    static  const char *const str = LAME_URL;

    return str;
}




void
get_lame_version_numerical(lame_version_t * lvp)
{
    static  const char *const features = ""; 

    
    lvp->major = LAME_MAJOR_VERSION;
    lvp->minor = LAME_MINOR_VERSION;
#if LAME_ALPHA_VERSION
    lvp->alpha = LAME_PATCH_VERSION;
    lvp->beta = 0;
#elif LAME_BETA_VERSION
    lvp->alpha = 0;
    lvp->beta = LAME_PATCH_VERSION;
#else
    lvp->alpha = 0;
    lvp->beta = 0;
#endif

    
    lvp->psy_major = PSY_MAJOR_VERSION;
    lvp->psy_minor = PSY_MINOR_VERSION;
    lvp->psy_alpha = PSY_ALPHA_VERSION;
    lvp->psy_beta = PSY_BETA_VERSION;

    
    
    lvp->features = features;
    
}


const char *
get_lame_os_bitness(void)
{
    static  const char *const strXX = "";
    static  const char *const str32 = "32bits";
    static  const char *const str64 = "64bits";

    switch (sizeof(void *)) {
    case 4:
        return str32;

    case 8:
        return str64;

    default:
        return strXX;
    }
}


