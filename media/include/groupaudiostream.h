#ifndef _GROUP_AUDIOSTREAM_H_
#define _GROUP_AUDIOSTREAM_H_

#include <pthread.h>
#include "config.h"
#include "codecs.h"
#include "rtpsession.h"
#include "audio.h"

class GroupAudioStream
{
public:
	GroupAudioStream();
	~GroupAudioStream();

	int Init(AudioInput *input,AudioOutput *output);
	int SetAudioCodec(AudioCodec::Type codec);
	RTPSession *StartSending(unsigned clientSessionId,char* sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap, RTPSession *rtp);
	int StopSending(unsigned clientSessionId);
	RTPSession *StartReceivingRtcp(unsigned clientSessionId);
	int End();

	int IsSending()	  { return sendingAudio;  }
	bool IsInited()	{ return inited;}
	MediaStatistics GetStatistics();

protected:
	int SendAudio();
	int RecvAudioRtcp();

private:
	//Funciones propias
	static void *startSendingAudio(void *par);
	static void *startRecvingAudioRtcp(void *par);
	AudioCodec* CreateAudioCodec(AudioCodec::Type type);

	// key= sessionid value=rtp session
	typedef std::map<unsigned,RTPSession*> Rtps;
	Rtps rtps;
	AudioInput	*audioInput;
	AudioOutput	*audioOutput;
	//pthread_mutex_t rtcp_mutex;
	//Rtps rtcps;

	//Parametros del audio
	AudioCodec::Type audioCodec;
	
	//Las threads
	pthread_t 	sendAudioThread;
	pthread_t 	recvAudioThread;

	//Controlamos si estamos mandando o no
	volatile int	sendingAudio;
	volatile int	recvingAudio;
	bool	inited;
};
#endif
