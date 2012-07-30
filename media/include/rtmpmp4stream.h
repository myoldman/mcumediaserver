#ifndef RTMPMP4STREAM_H
#define	RTMPMP4STREAM_H
#include "config.h"
#include "rtmp.h"
#include "rtmpstream.h"
#include "rtmpmessage.h"
#include "mp4streamer.h"
#include "audio.h"
#include <string>

class RTMPMP4Stream : public RTMPStream, public MP4Streamer::Listener
{
public:
	RTMPMP4Stream(DWORD id);
	virtual ~RTMPMP4Stream();
	virtual bool Play(std::wstring& url);
	virtual bool Seek(DWORD time);
	virtual bool Publish(std::wstring& url);
	virtual void PublishCommand(const wchar_t *name,AMFData* obj);
	virtual bool Close();

	/* MP4Streamer listener*/
	virtual void onRTPPacket(RTPPacket &packet);
	virtual void onTextFrame(TextFrame &text);
	virtual void onMediaFrame(MediaFrame &frame);
	virtual void onEnd();

private:
	MP4Streamer streamer;
	AVCDescriptor *desc;
	AudioCodec *decoder;
	AudioCodec *encoder;
};

#endif
