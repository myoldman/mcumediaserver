#ifndef _H264RTSPENCODER_H_
#define _H264RTSPENCODER_H_
#include "h264encoder.h"


class H264RtspEncoder : public H264Encoder
{
public:
	H264RtspEncoder(int qualityMin,int qualityMax);
	virtual ~H264RtspEncoder();
protected:
	virtual int OpenCodec();

};

#endif 

