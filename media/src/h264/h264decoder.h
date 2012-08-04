#ifndef _H264DECODER_H_
#define _H264DECODER_H_
extern "C" {
#include <libavcodec/avcodec.h>
} 
#include "codecs.h"
#include "video.h"

class H264Decoder : public VideoDecoder
{
public:
	H264Decoder();
	virtual ~H264Decoder();
	virtual int DecodePacket(BYTE *in,DWORD len,int lost,int last);
	virtual int Decode(BYTE *in,DWORD len);
	virtual int GetWidth()	{return ctx->width;};
	virtual int GetHeight()	{return ctx->height;};
	// add for rtsp watcher by liuhong start
	virtual int GetFPS() {return (int)round(1 / av_q2d(ctx->time_base))/2;};
	virtual BYTE* GetFrame(){return (BYTE *)frame;};
	virtual DWORD GetFrameSize(){return frameSize;};
	virtual int GetBitRate(){return ctx->bit_rate;};
	// add for rtsp watcher by liuhong end
private:
	AVCodec 	*codec;
	AVCodecContext	*ctx;
	AVFrame		*picture;
	BYTE*		buffer;
	DWORD		bufLen;
	DWORD 		bufSize;
	BYTE*		frame;
	DWORD		frameSize;
	BYTE		src;
	BYTE*		spsBuffer;
	DWORD		spsBufferLen;
	BYTE*		ppsBuffer;
	DWORD		ppsBufferLen;
	IDRPacket idrPacketTemp;
	IDRPacketSize idrPacketSizeTemp;
private:
	// add for rtsp watcher by liuhong start
	DWORD h264_append(BYTE *dest, DWORD destLen, DWORD destSize, BYTE *buffer, DWORD bufferLen, int last);
	DWORD h264_append_nals(BYTE *dest, DWORD destLen, DWORD destSize, BYTE *buffer, DWORD bufferLen, BYTE **nals, DWORD nalSize, DWORD *num, int last);
	void recontructIdrPacket();
	void copyIdrPacketFromTemp();
	// add for rtsp watcher by liuhong end
	
};
#endif
