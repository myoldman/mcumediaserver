#ifndef _GROUP_VIDEOSTREAM_H_
#define _GROUP_VIDEOSTREAM_H_

#include <pthread.h>
#include "config.h"
#include "codecs.h"
#include "rtpsession.h"
#include "RTPSmoother.h"
#include "video.h"
#include "rtmpstream.h"

class GroupVideoStream : public RTMPStreamPublisher
{
public:
	GroupVideoStream();
	~GroupVideoStream();

	virtual void SetPublishListener(PublishListener *listener);
	
	int Init(VideoInput *input, VideoOutput *output);

	int SetVideoCodec(VideoCodec::Type codec,int mode,int fps,int bitrate,int quality=0, int fillLevel=0,int intraPeriod=0);
	RTPSession *StartSending(unsigned clientSessionId,char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap, RTPSession *rtp);
	int StopSending(unsigned clientSessionId);
	RTPSession *StartReceivingRtcp(unsigned clientSessionId);
	int SendFPU();
	int End();

	// add for rtsp watcher by liuhong start
	int BroadcastPacket(BYTE * data, DWORD size, BYTE last, BYTE lost,
			VideoCodec::Type type, DWORD timestamp);

	int IsSending()	  { return sendingVideo;  }
	bool IsInited()	{ return inited;}

	MediaStatistics GetStatistics();
	
protected:
	int SendVideo();
	int RecVideo();
	int RecvVideoRtcp();

private:
	static void* startSendingVideo(void *par);
	static void *startRecvingVideoRtcp(void *par);
	//Funciones propias
	VideoEncoder* CreateVideoEncoder(VideoCodec::Type type);

	//Los objectos gordos
	VideoInput     	*videoInput;
	VideoOutput 	*videoOutput;
	// key= sessionid value=rtp session
	typedef std::map<unsigned,RTPSession*> Rtps;
	// key= sessionid value=rtp smoother
	typedef std::map<unsigned,RTPSmoother*> Smoothers;
	Rtps rtps;
	Smoothers smoothers;
	//Rtps rtcps;

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
	pthread_t 	recvVideoThread;
	pthread_mutex_t mutex;
	pthread_cond_t	cond;
	//pthread_mutex_t rtcp_mutex;

	//Controlamos si estamos mandando o no
	volatile int	sendingVideo;
	volatile int	recvingVideo;
	bool	inited;
	bool	sendFPU;
	PublishListener *listener;
	RTPDepacketizer* videoDepacketizer;

};

#endif
