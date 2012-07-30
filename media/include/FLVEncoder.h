#ifndef _FLVENCODER_H_
#define _FLVENCODER_H_
#include <pthread.h>
#include "video.h"
#include "audio.h"
#include "codecs.h"
#include "rtmpstream.h"

class FLVEncoder :
	public RTMPStreamPublisher
{
public:
	FLVEncoder();
	~FLVEncoder();

	virtual void SetPublishListener(PublishListener *listener);

	int Init(AudioInput* audioInput,VideoInput *videoInput);
	int StartEncoding();
	int StopEncoding();
	int End();

protected:
	int EncodeAudio();
	int EncodeVideo();

private:
	//Funciones propias
	static void *startEncodingAudio(void *par);
	static void *startEncodingVideo(void *par);
	
private:
	AudioCodec::Type	audioCodec;
	AudioInput*		audioInput;
	pthread_t		encodingAudioThread;
	int			encodingAudio;

	VideoInput*		videoInput;
	pthread_t		encodingVideoThread;
	int			encodingVideo;

	int		inited;
	timeval		ini;
	pthread_mutex_t mutex;
	PublishListener *listener;
};

#endif
