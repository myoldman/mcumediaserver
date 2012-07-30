#ifndef _VIDEOSTREAM_H_
#define _VIDEOSTREAM_H_

#include <pthread.h>
#include "config.h"
#include "codecs.h"
#include "rtpsession.h"
#include "RTPSmoother.h"
#include "video.h"
#include "groupvideostream.h"

class VideoMediaSubsession;
class RTPParticipant;

class VideoStream 
{
public:
	class Listener : public RTPSession::Listener
	{
	public:
		virtual void onRequestFPU() = 0;
	};
public:
	VideoStream(Listener* listener);
	~VideoStream();

	int Init(VideoInput *input, VideoOutput *output);
	int SetVideoCodec(VideoCodec::Type codec,int mode,int fps,int bitrate,int quality,int fillLevel,int intraPeriod);
	int StartSending(char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap);
	int StopSending();
	int SendFPU();
	int StartReceiving(VideoCodec::RTPMap& rtpMap);
	int StopReceiving();
	int SetMediaListener(MediaFrame::Listener *listener);
	int SetMute(bool isMuted);
	int End();

	int IsSending()	  { return sendingVideo;  }
	int IsReceiving() { return receivingVideo;}
	MediaStatistics GetStatistics();
	// add for rtsp watcher by liuhong start
	int SetParticipant(RTPParticipant *participant);
	VideoMediaSubsession* GetVideoMediaSubsession() { return videoSubsession;}
	void SetVideoMediaSubsession(VideoMediaSubsession* videoSubsession) { this->videoSubsession = videoSubsession;}
	RTPSession *InitVideoWatcher(unsigned clientSessionId);
	RTPSession *AddVideoWatcher(unsigned clientSessionId,char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap, VideoCodec::Type watcherCodec);
	int DeleteVideoWatcher(unsigned clientSessionId);
	VideoCodec::Type GetDecodeCodec(){return decodeCodec;};
	VideoMediaSubsession* CreateVideoSubsession(VideoCodec::Type videoCodec, int payloadType, char *sps_base64, char *pps_base64);
	RTPSession* GetRTPSession(){return &rtp;}
	void SetGroupVideoStream(GroupVideoStream* groupVideoStream) { this->m_GroupVideoStream = groupVideoStream;}
	IDRPacketSize GetIdrPacketSize()
	{
		if(videoDecoder == NULL)
		{
			IDRPacketSize size;
			return size;
		}
		return videoDecoder->idrPacketSize;
	}
	IDRPacket GetIdrPacket()
	{
		if(videoDecoder == NULL)
		{
			IDRPacket temp;
			return temp;
		}
		return videoDecoder->idrPacket;
	}
	DWORD GetCurrentTimestamp(){return currentTimestamp;}
	// add for rtsp watcher by liuhong end
	
protected:
	int SendVideo();
	int RecVideo();

private:
	static void* startSendingVideo(void *par);
	static void* startReceivingVideo(void *par);

	//Listners
	Listener* listener;
	MediaFrame::Listener *mediaListener;
	//Funciones propias
	VideoEncoder* CreateVideoEncoder(VideoCodec::Type type);

	//Los objectos gordos
	VideoInput     	*videoInput;
	VideoOutput 	*videoOutput;
	RTPSession      rtp;
	RTPSmoother	smoother;


	//Parametros del video

	VideoCodec::Type videoCodec;		//Codec de envio
	int		videoCaptureMode;	//Modo de captura de video actual
	int 		videoGrabWidth;		//Ancho de la captura
	int 		videoGrabHeight;	//Alto de la captur
	int 		videoFPS;
	int 		videoBitrate;
	int		videoQuality;
	int		videoFillLevel;
	int		videoIntraPeriod;

	//Las threads
	pthread_t 	sendVideoThread;
	pthread_t 	recVideoThread;
	pthread_mutex_t mutex;
	pthread_cond_t	cond;

	//Controlamos si estamos mandando o no
	bool	sendingVideo;	
	bool 	receivingVideo;
	bool	inited;
	bool	sendFPU;
	bool	muted;
	// add for rtsp watcher by liuhong begin
	//RTP Map types
	VideoMediaSubsession* videoSubsession;
	typedef std::map<unsigned,RTPSession*> Rtps;
	Rtps rtps;
	VideoCodec::Type decodeCodec;
	RTPParticipant *m_Participant;
	GroupVideoStream *m_GroupVideoStream;
	VideoDecoder *videoDecoder;
	DWORD currentTimestamp;
	//VideoEncoder*	tempEncoder;
	// add for rtsp watcher by liuhong end
};
#endif

