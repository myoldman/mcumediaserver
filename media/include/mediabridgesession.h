/* 
 * File:   mediabridgesession.h
 * Author: Sergio
 *
 * Created on 22 de diciembre de 2010, 18:20
 */

#ifndef _MEDIABRIDGESESSION_H_
#define	_MEDIABRIDGESESSION_H_
#include "rtpsession.h"
#include "rtmpstream.h"
#include "codecs.h"
#include "mp4recorder.h"
#include "waitqueue.h"

class MediaBridgeSession : public RTMPStream::PublishListener
{
public:
	MediaBridgeSession();
	~MediaBridgeSession();

	bool Init();
	//RTMP
	bool SetTransmitter(RTMPStreamPublisher *transmitter);
	void UnsetTransmitter();
	bool SetReceiver(RTMPStream *receiver);
	bool UnsetReceiver();
	//Video RTP
	int StartSendingVideo(char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap);
	int SetSendingVideoCodec(VideoCodec::Type codec);
	int SendFPU();
	int StopSendingVideo();
	int StartReceivingVideo(VideoCodec::RTPMap& rtpMap);
	int StopReceivingVideo();
	//Audio RTP
	int StartSendingAudio(char *sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap);
	int SetSendingAudioCodec(AudioCodec::Type codec);
	int StopSendingAudio();
	int StartReceivingAudio(AudioCodec::RTPMap& rtpMap);
	int StopReceivingAudio();
	//T140 Text RTP
	int StartSendingText(char *sendTextIp,int sendTextPort,TextCodec::RTPMap& rtpMap);
	int StopSendingText();
	int StartReceivingText(TextCodec::RTPMap& rtpMap);
	int StopReceivingText();

	bool End();

	virtual bool OnPublishedFrame(DWORD streamId,RTMPMediaFrame *frame);
	virtual bool OnPublishedMetaData(DWORD streamId,RTMPMetaData *meta);
	virtual bool OnPublishedCommand(uint32_t, const wchar_t*, AMFData*);

protected:
	int RecVideo();
	int RecAudio();
	int RecText();

	int SendVideo();
	int SendAudio();

	int DecodeAudio();

private:
	static void* startReceivingText(void *par);
	static void* startReceivingVideo(void *par);
	static void* startReceivingAudio(void *par);

	static void* startSendingVideo(void *par);
	static void* startSendingAudio(void *par);
	static void* startDecodingAudio(void *par);

private:
	//Bridged sessions
	RTMPStreamPublisher 	*transmitter;
	RTMPStream		*receiver;
	RTMPMetaData 	*meta;
	RTPSession      rtpAudio;
	RTPSession      rtpVideo;
	RTPSession      rtpText;

	WaitQueue<RTMPVideoFrame*> videoFrames;
	WaitQueue<RTMPAudioFrame*> audioFrames;
	
	VideoCodec::Type rtpVideoCodec;
	AudioCodec::Type rtpAudioCodec;

	AudioCodec *rtpAudioEncoder;
	AudioCodec *rtpAudioDecoder;
	AudioCodec *rtmpAudioEncoder;
	AudioCodec *rtmpAudioDecoder;

	//Las threads
	pthread_t 	recVideoThread;
	pthread_t 	recAudioThread;
	pthread_t 	recTextThread;
	pthread_t 	sendVideoThread;
	pthread_t 	sendAudioThread;
	pthread_t	decodeAudioThread;
	pthread_mutex_t	mutex;

	//Controlamos si estamos mandando o no
	bool	sendingVideo;
	bool 	receivingVideo;
	bool	sendingAudio;
	bool	receivingAudio;
	bool	sendingText;
	bool 	receivingText;
	bool	inited;
	bool	waitVideo;
	bool	sendFPU;
	timeval	first;
};

#endif	/* MEDIABRIDGESESSION_H */

