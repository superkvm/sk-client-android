
#ifndef LAME_ID3_H
#define LAME_ID3_H


#define CHANGED_FLAG    (1U << 0)
#define ADD_V2_FLAG     (1U << 1)
#define V1_ONLY_FLAG    (1U << 2)
#define V2_ONLY_FLAG    (1U << 3)
#define SPACE_V1_FLAG   (1U << 4)
#define PAD_V2_FLAG     (1U << 5)

enum {
    MIMETYPE_NONE = 0,
    MIMETYPE_JPEG,
    MIMETYPE_PNG,
    MIMETYPE_GIF,
};

typedef struct FrameDataNode {
    struct FrameDataNode *nxt;
    uint32_t fid;             
    char    lng[4];          
    struct {
        union {
            char   *l;       
            unsigned short *u; 
            unsigned char *b; 
        } ptr;
        size_t  dim;
        int     enc;         
    } dsc  , txt;
} FrameDataNode;


typedef struct id3tag_spec {
    
    unsigned int flags;
    int     year;
    char   *title;
    char   *artist;
    char   *album;
    char   *comment;
    int     track_id3v1;
    int     genre_id3v1;
    unsigned char *albumart;
    unsigned int albumart_size;
    unsigned int padding_size;
    int     albumart_mimetype;
    FrameDataNode *v2_head, *v2_tail;
} id3tag_spec;



extern int id3tag_write_v2(lame_global_flags * gfp);
extern int id3tag_write_v1(lame_global_flags * gfp);


#endif
