/* 
 * File:   mediagateway.h
 * Author: Sergio
 *
 * Created on 22 de diciembre de 2010, 18:10
 */

#ifndef _MEDIAGATEWAY_H_
#define	_MEDIAGATEWAY_H_
#include <map>
#include <string>
#include "pthread.h"
#include "mediabridgesession.h"
#include "rtmpstream.h"
#include "rtmpapplication.h"

class MediaGateway : public RTMPApplication
{
public:
	MediaGateway();
	~MediaGateway();

	bool Init();
	DWORD CreateMediaBridge(const std::wstring &name);
	bool  SetMediaBridgeInputToken(DWORD id,const std::wstring &token);
	bool  SetMediaBridgeOutputToken(DWORD id,const std::wstring &token);
	DWORD GetMediaBridgeIdFromInputToken(const std::wstring &token);
	DWORD GetMediaBridgeIdFromOutputToken(const std::wstring &token);
	bool  GetMediaBridgeRef(DWORD id,MediaBridgeSession **session);
	bool  ReleaseMediaBridgeRef(DWORD id);
	bool  DeleteMediaBridge(DWORD confId);
	bool End();

	/** RTMP application events */
	virtual RTMPStream* CreateStream(DWORD streamId,std::wstring &appName,DWORD audioCaps,DWORD videoCaps);

protected:


	class MediaBridgeInputStream : public RTMPStream
	{
	public:
		MediaBridgeInputStream(DWORD id,MediaGateway *mediaGateway);
		virtual ~MediaBridgeInputStream();
		virtual bool Play(std::wstring& url);
		virtual bool Seek(DWORD time);
		virtual bool Publish(std::wstring& url);
		virtual bool Close();
	private:
		MediaGateway *mediaGateway;
		DWORD sesId;
	};

	class MediaBridgeOutputStream : public RTMPStream
	{
	public:
		MediaBridgeOutputStream(DWORD id,MediaGateway *mediaGateway);
		virtual ~MediaBridgeOutputStream();
		virtual bool Play(std::wstring& url);
		virtual bool Seek(DWORD time);
		virtual void PlayMediaFrame(RTMPMediaFrame *frame);
		virtual bool Publish(std::wstring& url);
		virtual bool Close();
	private:
		MediaGateway *mediaGateway;
		DWORD sesId;
		QWORD first;
	};

	struct MediaBridgeEntry
	{
		DWORD id;
		DWORD numRef;
		DWORD enabled;
		std::wstring name;
                std::wstring inputToken;
		std::wstring outputToken;
		MediaBridgeSession* session;
	};
	typedef std::map<DWORD,MediaBridgeEntry> MediaBridgeEntries;
	typedef std::map<std::wstring,DWORD> InputMediaBridgesTokens;
	typedef std::map<std::wstring,DWORD> OutputMediaBridgesTokens;

private:
	MediaBridgeEntries		bridges;
	InputMediaBridgesTokens		inputTokens;
	OutputMediaBridgesTokens	outputTokens;
	DWORD			maxId;
	pthread_mutex_t		mutex;
	bool			inited;
};


#endif	/* MEDIAGATEWAY_H */

