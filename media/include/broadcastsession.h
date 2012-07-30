#ifndef _BROADCASTSESSION_H_
#define _BROADCASTSESSION_H_
#include "config.h"
#include "rtmpmessage.h"
#include "rtmpstream.h"
#include "flvrecorder.h"
#include <set>


class BroadcastSession : public RTMPStream::PublishListener
{
public:
	BroadcastSession(const std::wstring &tag);
	~BroadcastSession();
	bool Init(DWORD maxTransfer,DWORD maxConcurrent);
	bool SetTransmitter(RTMPStreamPublisher *transmitter);
	void UnsetTransmitter();
	bool AddReceiver(RTMPStream *receiver);
	bool RemoveReceiver(RTMPStream *receiver);
	bool End();

        virtual bool OnPublishedFrame(DWORD streamId,RTMPMediaFrame *frame);
	virtual bool OnPublishedMetaData(DWORD streamId,RTMPMetaData *meta);
	virtual bool OnPublishedCommand(DWORD id,const wchar_t *name,AMFData* obj);

private:
	typedef std::set<RTMPStream*> Receivers;
private:
	RTMPStreamPublisher 	*transmitter;
	RTMPMetaData 	*meta;
	Receivers	receivers;
	FLVRecorder	recorder;
	std::wstring	tag;
	
	//pthread_t	runThread;
	pthread_mutex_t	mutex;
	//pthread_cond_t	cond;
	bool		inited;
	DWORD		sent;
	DWORD		maxTransfer;
	DWORD		maxConcurrent;
};

#endif
