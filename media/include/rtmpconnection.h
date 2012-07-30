#ifndef _RTMPCONNECTION_H_
#define _RTMPCONNECTION_H_
#include <pthread.h>
#include <sys/poll.h>
#include "config.h"
#include "rtmp.h"
#include "rtmpchunk.h"
#include "rtmpmessage.h"
#include "rtmpstream.h"
#include "rtmpapplication.h"
#include <pthread.h>
#include <map>


class RTMPConnection : public RTMPStream::PlayListener
{
public:
	class Listener
	{
		public: 
			virtual RTMPApplication* OnConnect(const std::wstring& appName) = 0;
			virtual void onDisconnect(RTMPConnection *con) = 0;
	};
public:
	RTMPConnection(Listener* listener);
	~RTMPConnection();

	int Init(int fd);
	int End();

protected:
	int Run();
	
private:
	static  void* run(void *par);

	void ParseData(BYTE *data,const DWORD size);
	DWORD SerializeChunkData(BYTE *data,const DWORD size);
	int WriteData(BYTE *data,const DWORD size);

	void ProcessControlMessage(DWORD messageStremId,BYTE type,RTMPObject* msg);
	void ProcessCommandMessage(DWORD messageStremId,RTMPCommandMessage* cmd);
	void ProcessMediaData(DWORD messageStremId,RTMPMediaFrame* frame);
	void ProcessMetaData(DWORD messageStremId,RTMPMetaData* frame);

	void SendCommandError(DWORD messageStreamId,QWORD transId,AMFData* params = NULL,AMFData *extra = NULL);
	void SendCommandResult(DWORD messageStreamId,QWORD transId,AMFData* params,AMFData *extra);
	void SendCommandResponse(DWORD messageStreamId,const wchar_t* name,QWORD transId,AMFData* params,AMFData *extra);
	
	void SendCommand(DWORD messageStreamId,const wchar_t* name,AMFData* params,AMFData *extra);

	//void SendVideoData(DWORD messageStremId,BYTE *data,const DWORD size,DWORD timestamp);
	//void SendAudioData(DWORD messageStremId,BYTE *data,const DWORD size,DWORD timestamp);
	void SendControlMessage(RTMPMessage::Type type,RTMPObject* msg);
	
	void SignalWriteNeeded();

	//Listener for the media data
	virtual void OnPlayedFrame(DWORD id,RTMPMediaFrame* frame);
	virtual void OnPlayedMetaData(DWORD id,RTMPMetaData* meta);
	virtual void OnStreamBegin(DWORD id);
	virtual void OnStreamIsRecorded(DWORD id);
	virtual void OnStreamEnd(DWORD id);
	virtual void OnStreamReset(DWORD id);
	virtual void OnCommand(DWORD id,const wchar_t *name,AMFData* obj);

private:
	enum State {HEADER_C0_WAIT=0,HEADER_C1_WAIT=1,HEADER_C2_WAIT=2,CHUNK_HEADER_WAIT=3,CHUNK_TYPE_WAIT=4,CHUNK_EXT_TIMESTAMP_WAIT=5,CHUNK_DATA_WAIT=6};
	typedef std::map<DWORD,RTMPChunkInputStream*>  RTMPChunkInputStreams;
	typedef std::map<DWORD,RTMPChunkOutputStream*> RTMPChunkOutputStreams;
	typedef std::map<DWORD,RTMPStream*> RTMPStreams;

private:
	int socket;
	pollfd ufds[1];
	int inited;
	State state;

	RTMPHandshake01 s01;
	RTMPHandshake0 c0;
	RTMPHandshake1 c1;
	RTMPHandshake2 s2;
	RTMPHandshake2 c2;
	
	bool digest;

	DWORD videoCodecs;
	DWORD audioCodecs;

	RTMPChunkBasicHeader header;
	RTMPChunkType0	type0;
	RTMPChunkType1	type1;
	RTMPChunkType2	type2;
	RTMPExtendedTimestamp extts;

	
	RTMPChunkInputStreams  	chunkInputStreams;
	RTMPChunkOutputStreams  chunkOutputStreams;
	RTMPChunkInputStream* 	chunkInputStream;

	DWORD chunkStreamId;
	DWORD chunkLen;
	DWORD maxChunkSize;
	DWORD maxOutChunkSize;

	pthread_t thread;
	pthread_mutex_t mutex;

	RTMPApplication* app;
	std::wstring	appName;
	RTMPStreams streams;
	DWORD maxStreamId;
	DWORD maxTransId;

	Listener* listener;

	timeval startTime;
	DWORD windowSize;
	DWORD curWindowSize;
	DWORD recvSize;
	DWORD inBytes;
	DWORD outBytes;
};

#endif
