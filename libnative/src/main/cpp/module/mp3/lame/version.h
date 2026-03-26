

#ifndef LAME_VERSION_H
#define LAME_VERSION_H



#ifndef STR
# define __STR(x)  #x
# define STR(x)    __STR(x)
#endif

# define LAME_URL              "http://lame.sf.net"


# define LAME_MAJOR_VERSION      3 
# define LAME_MINOR_VERSION     99 
# define LAME_TYPE_VERSION       2 
# define LAME_PATCH_VERSION      5 
# define LAME_ALPHA_VERSION     (LAME_TYPE_VERSION==0)
# define LAME_BETA_VERSION      (LAME_TYPE_VERSION==1)
# define LAME_RELEASE_VERSION   (LAME_TYPE_VERSION==2)

# define PSY_MAJOR_VERSION       1 
# define PSY_MINOR_VERSION       0 
# define PSY_ALPHA_VERSION       0 
# define PSY_BETA_VERSION        0 

#if LAME_ALPHA_VERSION
#define LAME_PATCH_LEVEL_STRING " alpha " STR(LAME_PATCH_VERSION)
#endif
#if LAME_BETA_VERSION
#define LAME_PATCH_LEVEL_STRING " beta " STR(LAME_PATCH_VERSION)
#endif
#if LAME_RELEASE_VERSION
#if LAME_PATCH_VERSION
#define LAME_PATCH_LEVEL_STRING " release " STR(LAME_PATCH_VERSION)
#else
#define LAME_PATCH_LEVEL_STRING ""
#endif
#endif

# define LAME_VERSION_STRING STR(LAME_MAJOR_VERSION) "." STR(LAME_MINOR_VERSION) LAME_PATCH_LEVEL_STRING

#endif 


