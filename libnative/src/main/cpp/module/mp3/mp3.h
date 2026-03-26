

#ifndef SUPERKVM_MP3_H
#define SUPERKVM_MP3_H

#ifdef __cplusplus
extern "C" {
#endif
void lameInitInternal(int inSampleRate, int outChannel, int outSampleRate, int outBitRate, int quality);
int lameEncodeInternal(short* leftBuf, short* rightBuf, int sampleRate, unsigned char* mp3Buf, int len);
int lameFlushInternal(unsigned char* mp3Buf, int len);
void lameCloseInternal();
#ifdef __cplusplus
};
#endif
#endif 
