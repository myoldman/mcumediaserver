#ifndef _RTPSESSION_H_
#define _RTPSESSION_H_
#include "config.h"
#include "codecs.h"
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <use.h>
#include "rtp.h"
#include "UsageEnvironment.hh"

struct MediaStatistics
{
	bool		isSending;
	bool		isReceiving;
	DWORD		lostRecvPackets;
	DWORD		numRecvPackets;
	DWORD		numSendPackets;
	DWORD		totalRecvBytes;
	DWORD		totalSendBytes;
};

class RTPSession
{
public:
	class Listener
	{
	public:
		virtual void onFPURequested(RTPSession *session) = 0;
	};
public:
	RTPSession(Listener *listener);
	~RTPSession();
	int Init();
	int GetLocalPort();
	int SetRemotePort(char *ip,int sendPort);
	
	int End();

	void SetSendingVideoRTPMap(VideoCodec::RTPMap &map);
	void SetSendingAudioRTPMap(AudioCodec::RTPMap &map);
	void SetSendingTextRTPMap(TextCodec::RTPMap &map);
	void SetReceivingVideoRTPMap(VideoCodec::RTPMap &map);
	void SetReceivingAudioRTPMap(AudioCodec::RTPMap &map);
	void SetReceivingTextRTPMap(TextCodec::RTPMap &map);

	bool SetSendingVideoCodec(VideoCodec::Type codec);
	bool SetSendingAudioCodec(AudioCodec::Type codec);
	bool SetSendingTextCodec(TextCodec::Type codec);

	int SendEmptyPacket();
	int SendVideoPacket(BYTE* data,int size,int last,DWORD frameTime);
	int SendAudioPacket(BYTE* data,int size,DWORD frameTime);
	int SendTextPacket(BYTE* data,int size,DWORD timestamp,bool mark);
	int SendPacket(RTPPacket &packet,DWORD timestamp);
	int ForwardPacket(BYTE* data,int size, int last, DWORD ts);
	int GetVideoPacket(BYTE *data,DWORD *size,BYTE *last,BYTE *lost,VideoCodec::Type *codec,DWORD *timestamp);
	int GetAudioPacket(BYTE* data,DWORD *size,BYTE *lost,AudioCodec::Type *codec,DWORD *timestamp);
	int GetTextPacket(BYTE* data,DWORD *size,BYTE *lost,TextCodec::Type *codec,DWORD *timestamp);
	// added for rtcp recieve
	int GetRtcpPacket(BYTE* data,DWORD *size);
	int SendRtcpSRPacket();

	DWORD GetNumRecvPackets()	{ return numRecvPackets;	}
	DWORD GetNumSendPackets()	{ return numSendPackets;	}
	DWORD GetTotalRecvBytes()	{ return totalRecvBytes;	}
	DWORD GetTotalSendBytes()	{ return totalSendBytes;	}
	DWORD GetLostRecvPackets()	{ return lostRecvPackets;	}
	
	// rtsp watcher
	bool IsSendFPU()	{ return sendFPU;}
	void SendFPU()	{ sendFPU = true;}
	bool IsFromRtsp()	{ return fromRtsp;}
	void FromRtsp()	{ fromRtsp = true;}
	void SetRtcpRRHandler(TaskFunc* rtcpRRHandler, void* rtcpRRHandlerClientData){
	this->rtcpRRHandler = rtcpRRHandler;
	this->rtcpRRHandlerClientData = rtcpRRHandlerClientData;}
	int RecvRtcp();
	bool IsInited(){return recvingRtcp;}
private:
	void SetSendingType(int type);
	void CreatePacket(BYTE* buffer,int type);
	void CreateRtcpPacket(BYTE* buffer,int type);
	inline int WaitForSocket(int fd, int secs);
	
protected:
	//Envio y recepcion de rtcp
	int SendRtcp();
	static void *startRecvingRtcp(void *par);

private:
	Listener* listener;
	//Sockets
	int 	simSocket;
	int 	simRtcpSocket;
	int 	simPort;
	int	simRtcpPort;

	//Tipos
	int 	sendType;

	//Transmision
	sockaddr_in sendAddr;
	sockaddr_in sendRtcpAddr;
	BYTE 	sendPacket[MTU];
	WORD    sendSeq;
	DWORD   sendTime;

	//Recepcion
	BYTE	recBuffer[MTU];
	BYTE	recRTCPBuffer[MTU];
	int	recSeq;
	DWORD	recSSRC;
	in_addr_t recIP;
	DWORD	  recPort;

	//RTP Map types
	VideoCodec::RTPMap* rtpVideoMapIn;
	VideoCodec::RTPMap* rtpVideoMapOut;
	AudioCodec::RTPMap* rtpAudioMapIn;
	AudioCodec::RTPMap* rtpAudioMapOut;
	TextCodec::RTPMap* rtpTextMapIn;
	TextCodec::RTPMap* rtpTextMapOut;

	//Statistics
	DWORD		numRecvPackets;
	DWORD		numSendPackets;
	DWORD		totalRecvBytes;
	DWORD		totalSendBytes;
	DWORD		lostRecvPackets;

	// for rtsp watcher
	bool sendFPU;
	bool fromRtsp;
	BYTE forwardPacket[MTU];
	BYTE rtcpPacket[MTU];
	in_addr_t recRTCPIP;
	DWORD	  recRTCPPort;
	pthread_t 	recvRtcpThread;
	volatile int	recvingRtcp;
	TaskFunc* rtcpRRHandler;
	void* rtcpRRHandlerClientData;
};

#endif
