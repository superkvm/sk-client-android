



#ifndef _SYS_MD5_H_
#define _SYS_MD5_H_

#define MD5_BLOCK_LENGTH		64
#define MD5_DIGEST_LENGTH		16
#define MD5_DIGEST_STRING_LENGTH	(MD5_DIGEST_LENGTH * 2 + 1)


typedef struct MD5Context {
  unsigned int state[4];	
  unsigned int count[2];	
  unsigned char buffer[64];	
} MD5_CTX;

void   MD5Init (MD5_CTX *);
void   MD5Update (MD5_CTX *, const void *, unsigned int);
void   MD5Final (unsigned char [16], MD5_CTX *);
char * MD5End(MD5_CTX *, char *);
char * MD5File(const char *, char *);
char * MD5FileChunk(const char *, char *, off_t, off_t);
char * MD5Data(const void *, unsigned int, char *);
#endif 
