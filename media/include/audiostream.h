#ifndef _AUDIOSTREAM_H_
#define _AUDIOSTREAM_H_

#include <pthread.h>
#include "config.h"
#include "codecs.h"
#include "rtpsession.h"
#include "audio.h"

//class AudioMediaSubsession;
class RTPParticipant;

class AudioStream
{
public:
	AudioStream(RTPSession::Listener* listener);
	~AudioStream();

	int Init(AudioInput *input,AudioOutput *output);
	int SetAudioCodec(AudioCodec::Type codec);
	int StartSending(char* sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap);
	int StopSending();
	int StartReceiving(AudioCodec::RTPMap& rtpMap);
	int StopReceiving();
	int SetMute(bool isMuted);
	int End();

	int IsSending()	  { return sendingAudio;  }
	int IsReceiving() { return receivingAudio;}
	MediaStatistics GetStatistics();
	// add for rtsp watcher by liuhong start
	int SetParticipant(RTPParticipant *participant);
	//AudioMediaSubsession* GetAudioMediaSubsession() { return audioSubsession;}
	//void SetAudioMediaSubsession(AudioMediaSubsession* audioSubsession) { this->audioSubsession = audioSubsession;}
	RTPSession *InitAudioWatcher(unsigned clientSessionId);
	RTPSession *AddAudioWatcher(unsigned clientSessionId,char *sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap, AudioCodec::Type watcherCodec);
	int DeleteAudioWatcher(unsigned clientSessionId);
	AudioCodec::Type GetDecodeCodec(){return decodeCodec;};
	//AudioMediaSubsession* CreateAudioSubsession(AudioCodec::Type audioCodec, int payloadType);
	RTPSession* GetRTPSession(){return &rtp;}
	// add for rtsp watcher by liuhong end

protected:
	int SendAudio();
	int RecAudio();

private:
	//Funciones propias
	static void *startSendingAudio(void *par);
	static void *startReceivingAudio(void *par);
	AudioCodec* CreateAudioCodec(AudioCodec::Type type);

	//Los objectos gordos
	RTPSession	rtp;
	AudioInput	*audioInput;
	AudioOutput	*audioOutput;

	//Parametros del audio
	AudioCodec::Type audioCodec;
	
	//Las threads
	pthread_t 	recAudioThread;
	pthread_t 	sendAudioThread;

	//Controlamos si estamos mandando o no
	volatile int	sendingAudio;
	volatile int 	receivingAudio;

	bool		muted;	
	// add for rtsp watcher by liuhong begin
	//AudioMediaSubsession* audioSubsession;
	typedef std::map<unsigned,RTPSession*> Rtps;
	Rtps rtps;
	AudioCodec::Type decodeCodec;
	RTPParticipant *m_Participant;
	// add for rtsp watcher by liuhong end
};
#endif
