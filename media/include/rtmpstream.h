#ifndef _RTMPSTREAM_H_
#define _RTMPSTREAM_H_
#include "config.h"
#include "rtmpmessage.h"
#include <string>
#include <list>

class RTMPStreamPublisher
{
public:
	class PublishListener
	{
		public: virtual bool OnPublishedFrame(DWORD id,RTMPMediaFrame *frame) = 0;
		public: virtual bool OnPublishedMetaData(DWORD id,RTMPMetaData *meta) = 0;
		public: virtual bool OnPublishedCommand(DWORD id,const wchar_t *name,AMFData* obj) = 0;
	};
public:
	virtual void SetPublishListener(PublishListener* listener) = 0;
};

class RTMPStreamPlayer
{
public:
	class PlayListener
	{
		public: virtual void OnPlayedFrame(DWORD id,RTMPMediaFrame *frame) = 0;
		public: virtual void OnPlayedMetaData(DWORD id,RTMPMetaData *meta) = 0;
		public: virtual void OnStreamBegin(DWORD id) = 0;
		public: virtual void OnStreamEnd(DWORD id) = 0;
		public: virtual void OnStreamIsRecorded(DWORD id) = 0;
		public: virtual void OnStreamReset(DWORD id) = 0;
		public: virtual void OnCommand(DWORD id,const wchar_t *name,AMFData* obj) = 0;
	};
public:
	virtual void SetPlayListener(PlayListener* listener) = 0;
};


class RTMPStream : 
	public RTMPStreamPublisher,
	public RTMPStreamPlayer
{
public:

	RTMPStream(DWORD id);
	virtual ~RTMPStream();
	virtual bool Play(std::wstring& url) = 0;
	virtual bool Seek(DWORD time) = 0;
	virtual bool Close() = 0;

	virtual void SetPublishListener(PublishListener* listener);
	virtual void SetPlayListener(PlayListener* listener);

	virtual bool Publish(std::wstring& url) = 0;
	virtual void PublishMediaFrame(RTMPMediaFrame *frame);
	virtual void PublishMetaData(RTMPMetaData *meta);
	virtual void PublishCommand(const wchar_t *name,AMFData* obj);
	
	virtual void PlayMetaData(RTMPMetaData *meta);
	virtual void PlayMediaFrame(RTMPMediaFrame *frame);
	virtual void SendCommand(const wchar_t *name,AMFData* obj);
	virtual void SendStreamBegin();
	virtual void SendStreamIsRecorded();
	virtual void SendStreamEnd();
	virtual void Reset();

protected:
	PublishListener* publishListener;
	PlayListener*	 playListener;
	DWORD	id;
};

#endif
