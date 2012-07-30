#ifndef _BROADCASTER_H_
#define _BROADCASTER_H_
#include <map>
#include <string>
#include "pthread.h"
#include "broadcastsession.h"
#include "rtmpstream.h"
#include "rtmpapplication.h"

class Broadcaster : public RTMPApplication
{
public:
	Broadcaster();
	~Broadcaster();

	bool Init();
	bool End();

	DWORD CreateBroadcast(const std::wstring &name,const std::wstring &tag);
	bool  PublishBroadcast(DWORD id,const std::wstring &pin);
	DWORD GetPublishedBroadcastId(const std::wstring &pin);
	bool  AddBroadcastToken(DWORD id,const std::wstring &token);
	DWORD GetTokenBroadcastId(const std::wstring &token);
	bool  UnPublishBroadcast(DWORD id);
	bool  GetBroadcastRef(DWORD id,BroadcastSession **broadcast);
	bool  ReleaseBroadcastRef(DWORD id);
	bool  DeleteBroadcast(DWORD confId);

	
	/** RTMP application events */
	virtual RTMPStream* CreateStream(DWORD streamId,std::wstring &appName,DWORD audioCaps,DWORD videoCaps);

protected:


	class BroadcastInputStream : public RTMPStream
	{
	public:
		BroadcastInputStream(DWORD id,Broadcaster *broadcaster);
		virtual ~BroadcastInputStream();
		virtual bool Play(std::wstring& url);
		virtual bool Seek(DWORD time);
		virtual bool Publish(std::wstring& url);
		virtual bool Close();
	private:
		Broadcaster *broadcaster;
		DWORD sesId;
	};

	class BroadcastOutputStream : public RTMPStream
	{	
	public:
		BroadcastOutputStream(DWORD id,Broadcaster *broadcaster);
		virtual ~BroadcastOutputStream();
		virtual bool Play(std::wstring& url);
		virtual bool Seek(DWORD time);
		virtual void PlayMediaFrame(RTMPMediaFrame *frame);
		virtual bool Publish(std::wstring& url);
		virtual bool Close();
	private:
		Broadcaster *broadcaster;
		DWORD sesId;
		QWORD first;
	};

	struct BroadcastEntry
	{
		DWORD id;
		DWORD numRef;
		DWORD enabled;
		std::wstring name;
                std::wstring tag;
		std::wstring pin;
		BroadcastSession* session;
	};
	typedef std::map<DWORD,BroadcastEntry> BroadcastEntries;
	typedef std::map<std::wstring,DWORD> PublishedBroadcasts;
	typedef std::map<std::wstring,DWORD> BroadcastTokens;

private:
	BroadcastEntries	broadcasts;
	PublishedBroadcasts	published;
	BroadcastTokens		tokens;
	DWORD			maxId;
	pthread_mutex_t		mutex;
	bool inited;
};

#endif
