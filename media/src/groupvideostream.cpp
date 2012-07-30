#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include "groupvideostream.h"
#include "h263/h263codec.h"
#include "h263/mpeg4codec.h"
#include "h264/h264encoder.h"
#include "h264/h264decoder.h"
#include "h264/h264depacketizer.h"
#include "log.h"
#include "tools.h"
#include "RTPSmoother.h"


/**********************************
* VideoStream
*	Constructor
***********************************/
GroupVideoStream::GroupVideoStream()
{
	//Inicializamos a cero todo
	sendVideoThread = 0;
	sendingVideo=0;
	recvVideoThread = 0;
	recvingVideo=0;
	videoInput=NULL;
	videoOutput=NULL;
	videoCodec=VideoCodec::H263_1996;
	videoCaptureMode=0;
	videoGrabWidth=0;
	videoGrabHeight=0;
	videoFPS=0;
	videoBitrate=0;
	videoQuality=0;
	videoFillLevel=0;
	videoIntraPeriod=0;
	sendFPU = false;
	inited = false;
	videoDepacketizer = NULL;
	//Create objects
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&cond,NULL);
	//add for rtsp by liuhong
	//pthread_mutex_init(&rtcp_mutex, NULL);
}

/*******************************
* ~GroupVideoStream
*	Destructor. Cierra los dispositivos
********************************/
GroupVideoStream::~GroupVideoStream()
{
	//Clean object
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	//pthread_mutex_destroy(&rtcp_mutex);
}

/**********************************************
* SetVideoCodec
*	Fija el modo de envio de video 
**********************************************/
int GroupVideoStream::SetVideoCodec(VideoCodec::Type codec,int mode,int fps,int bitrate,int quality, int fillLevel,int intraPeriod)
{
	Log("-SetVideoCodec [%s,%dfps,%dkbps,qMax:%d,qMin%d,intra:%d]\n",VideoCodec::GetNameFor(codec),fps,bitrate,quality,fillLevel,intraPeriod);

	//Dependiendo del tipo ponemos las calidades por defecto
	switch(codec)
	{
		case VideoCodec::H263_1996:
			videoQuality=1;
			videoFillLevel=6;
			break;
		case VideoCodec::H263_1998:
			videoQuality=1;
			videoFillLevel=6;
			break;
		case VideoCodec::H264:
			videoQuality=1;
			videoFillLevel=6;
			break;
		default:
			//Return codec
			return Error("Codec not found\n");
	}

	//LO guardamos
	videoCodec=codec;

	//Guardamos el bitrate
	videoBitrate=bitrate;

	//La calidad
	if (quality>0)
		videoQuality = quality;

	//El level
	if (fillLevel>0)
		videoFillLevel = fillLevel;

	//The intra period
	if (intraPeriod>0)
		videoIntraPeriod = intraPeriod;

	//Get width and height
	videoGrabWidth = GetWidth(mode);
	videoGrabHeight = GetHeight(mode);
	//Check size
	if (!videoGrabWidth || !videoGrabHeight)
		//Error
		return Error("Unknown video mode\n");

	//Almacenamos el modo de captura
	videoCaptureMode=mode;

	//Y los fps
	videoFPS=fps;
	
	inited = true;

	return 1;
}

/***************************************
* Init
*	Inicializa los devices 
***************************************/
int GroupVideoStream::Init(VideoInput *input,VideoOutput *output)
{
	Log(">Init video stream\n");

	//Guardamos los objetos
	videoInput  = input;
	videoOutput = output;
	
	//No estamos haciendo nada
	sendingVideo=0;
	recvingVideo=0;
	
	listener = NULL;
	Log("<Init video stream end\n");

	return 1;
}

int GroupVideoStream::RecvVideoRtcp()
{
	/*
	BYTE	data[MTU];
	DWORD		size;
	Log(">RecvVideoRtcp\n");
	while(recvingVideo)
	{
		pthread_mutex_lock(&rtcp_mutex);
		Rtps::iterator iter;
		for(iter = rtcps.begin(); iter != rtcps.end(); iter++)
		{
			RTPSession* rtp = iter->second;
			if(rtp && rtp->GetRtcpPacket(data, &size)) {
				int sr_size = rtp->SendRtcpSRPacket();
			}
		}
		pthread_mutex_unlock(&rtcp_mutex);
	}
	Log("<RecvVideoRtcp end\n");
	*/
	pthread_exit(0);
}

void* GroupVideoStream::startRecvingVideoRtcp(void *par)
{
	Log("startRecvingVideoRtcp [%d]\n",getpid());

	//OBtenemos el objeto
	GroupVideoStream *conf = (GroupVideoStream *)par;

	//Bloqueamos las se�ales
	blocksignals();

	//Y ejecutamos la funcion
	pthread_exit((void *)conf->RecvVideoRtcp());
}

RTPSession *GroupVideoStream::StartReceivingRtcp(unsigned clientSessionId)
{
	Log(">StartReceivingRtcp Group video [%u]\n",clientSessionId);
	
	pthread_mutex_lock(&mutex);
	Rtps::iterator it = rtps.find(clientSessionId);
	RTPSession *rtp = NULL;
	if (it != rtps.end()) {
		Error("client already opened\n");
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	rtp = new RTPSession(NULL);
	if(!rtp->Init())
	{
		delete rtp;
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	rtp->FromRtsp();
	rtps[clientSessionId] = rtp;
	pthread_mutex_unlock(&mutex);
	
	/*
	pthread_mutex_lock(&rtcp_mutex);
	rtcps[clientSessionId] = rtp;
	pthread_mutex_unlock(&rtcp_mutex);
	*/

	if(recvingVideo == 0)
	{
		//Arrancamos el thread de envio
		recvingVideo=1;
		//Start thread
		createPriorityThread(&recvVideoThread,startRecvingVideoRtcp,this,1);
	}

	Log("<StartReceivingRtcp video [%u]\n",clientSessionId);

	return rtp;
}


/**************************************
* startSendingVideo
*	Function helper for thread
**************************************/
void* GroupVideoStream::startSendingVideo(void *par)
{
	Log("SendVideoThread [%d]\n",getpid());

	//OBtenemos el objeto
	GroupVideoStream *conf = (GroupVideoStream *)par;

	//Bloqueamos las se�ales
	blocksignals();

	//Y ejecutamos la funcion
	pthread_exit((void *)conf->SendVideo());
}

/***************************************
* StartSending
*	Comienza a mandar a la ip y puertos especificados
***************************************/
RTPSession *GroupVideoStream::StartSending(unsigned clientSessionId, char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap, RTPSession *rtp)
{
	Log(">StartSending Group Video [%u, %s,%d]\n",clientSessionId, sendVideoIp,sendVideoPort);

	//Si tenemos video
	if (sendVideoPort==0)
	{
		Error("No video\n");
		return NULL;
	}
	
	pthread_mutex_lock(&mutex);

	Rtps::iterator it = rtps.find(clientSessionId);

	//Si no esta
	if (it != rtps.end())
	{
		rtp = (*it).second;
	}
	
	if(rtp == NULL)
	{
		rtp = new RTPSession(NULL);
		if(!rtp->Init())
		{
			delete rtp;
			Error("Rtp init error\n");
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		rtp->FromRtsp();
	}
	
	//Iniciamos las sesiones rtp de envio
	if(!rtp->SetRemotePort(sendVideoIp,sendVideoPort))
	{
		if(rtp->IsFromRtsp()) {
			rtp->End();
			delete rtp;
		}
		Error("Rtp SetRemotePort error\n");
		pthread_mutex_unlock(&mutex);
		return NULL;
	}
	
	rtp->SetSendingVideoRTPMap(rtpMap);
	
	//Set video codec
	if(!rtp->SetSendingVideoCodec(videoCodec))
	{
		if(rtp->IsFromRtsp()) {
			rtp->End();
			delete rtp;
		}
		//Error
		Error("%s video codec not supported by peer\n",VideoCodec::GetNameFor(videoCodec));
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	rtps[clientSessionId] = rtp;
	RTPSmoother* smoother = new RTPSmoother();
	smoother->Init(rtp);
	smoothers[clientSessionId] = smoother;
	SendFPU();

	pthread_mutex_unlock(&mutex);

	if(sendingVideo == 0)
	{
		sendingVideo=1;
		//createPriorityThread(&sendVideoThread,startSendingVideo,this,0);
	}

	/*
	pthread_mutex_lock(&rtcp_mutex);
	if(rtcps[clientSessionId] == NULL)
		rtcps[clientSessionId] = rtp;
	pthread_mutex_unlock(&rtcp_mutex);
	
	if(recvingVideo == 0)
	{
		//Arrancamos el thread de envio
		recvingVideo=1;
		//Start thread
		createPriorityThread(&recvVideoThread,startRecvingVideoRtcp,this,1);
	}
	*/
	//LOgeamos
	Log("<StartSending video [%u]\n",clientSessionId);

	return rtp;
}

/***************************************
* End
*	Termina la conferencia activa
***************************************/
int GroupVideoStream::End()
{
	Log(">End\n");

	if (sendingVideo)
	{
		while(rtps.size()>0)
			StopSending(rtps.begin()->first);
		delete videoDepacketizer;
	}

	Log("<GroupVideoStream End\n");

	return 1;
}

/***************************************
* StopSending
*	Termina el envio
***************************************/
int GroupVideoStream::StopSending(unsigned clientSessionId)
{
	Log(">StopSending Group Video [%u]\n",clientSessionId);

	/*
	pthread_mutex_lock(&rtcp_mutex);
	Log(">rtcp_mutex lock\n");
	Rtps::iterator it_rtcp = rtcps.find(clientSessionId);
	if (it_rtcp != rtcps.end())
	{
		rtcps.erase(it_rtcp);
	}
	pthread_mutex_unlock(&rtcp_mutex);
	*/

	pthread_mutex_lock(&mutex);
	//El iterator
	Rtps::iterator it = rtps.find(clientSessionId);
	//Si no esta
	if (it == rtps.end())
	{
		pthread_mutex_unlock(&mutex);
		return Error("Rtps not found\n");
	}


	RTPSession* rtp = (*it).second;
	rtps.erase(it);
	Smoothers::iterator it2 = smoothers.find(clientSessionId);
	
	RTPSmoother* smoother = (*it2).second;
	smoothers.erase(it2);
	smoother->End();
	rtp->End();
	// delete it because it create from rtsp session
	if(rtp->IsFromRtsp())
		delete rtp;

	delete smoother;

	pthread_mutex_unlock(&mutex);

	Log(">after stop send [%u]\n", smoothers.size());
	if(smoothers.size() == 0)
	{
		sendingVideo = 0;
		videoInput->CancelGrabFrame();
		pthread_cond_signal(&cond);
		if(sendVideoThread != 0) {
			pthread_join(sendVideoThread,NULL);
			sendVideoThread = 0;
			Log(">pthread_join send [%u]\n", sendVideoThread);
		}
	}
	
	/*
	if(rtcps.size() == 0)
	{
		recvingVideo=0;
		if(recvVideoThread!=0) {
			pthread_join(recvVideoThread,NULL);
			recvVideoThread = 0;
		}
	}
	*/

	Log(">StopSending [%u,%u]\n",clientSessionId, rtps.size());

	return 1;
}

// add for rtsp watcher by liuhong start
int GroupVideoStream::BroadcastPacket(BYTE* data, DWORD size, BYTE last, BYTE lost,
			VideoCodec::Type type, DWORD timestamp)
{
	//Log(">BroadcastPacket  %ld %d\n", size, last);
	
	pthread_mutex_lock(&mutex);
	Rtps::iterator iter;
	for(iter = rtps.begin(); iter != rtps.end(); iter++)
	{
		RTPSession* rtp = iter->second;
		rtp->ForwardPacket(data, size, last, timestamp);
	}
	pthread_mutex_unlock(&mutex);
	return 1;
}
// add for rtsp watcher by liuhong end

/*******************************************
* SendVideo
*	Capturamos el video y lo mandamos
*******************************************/
int GroupVideoStream::SendVideo()
{
	timeval first;
	timeval prev;
	
	Log(">SendVideo [%d,%d,%d,%d,%d,%d,%d]\n",videoGrabWidth,videoGrabHeight,videoBitrate,videoFPS,videoQuality,videoFillLevel,videoIntraPeriod);

	//Creamos el encoder
	VideoEncoder* videoEncoder = videoEncoder = VideoCodecFactory::CreateEncoder(videoCodec);

	//Comprobamos que se haya creado correctamente
	if (videoEncoder == NULL)
		//error
		return Error("Can't create video encoder\n");

	//Comrpobamos que tengamos video de entrada
	if (videoInput == NULL)
		return Error("No video input");

	//Iniciamos el tama�o del video
	if (!videoInput->StartVideoCapture(videoGrabWidth,videoGrabHeight,videoFPS))
		return Error("Couldn't set video capture\n");

	//Iniciamos el birate y el framerate
	videoEncoder->SetFrameRate((float)videoFPS,videoBitrate,videoIntraPeriod);

	//No wait for first
	DWORD frameTime = 0;

	//Iniciamos el tamama�o del encoder
 	videoEncoder->SetSize(videoGrabWidth,videoGrabHeight);

	//The time of the first one
	gettimeofday(&first,NULL);

	//The time of the previos one
	gettimeofday(&prev,NULL);
	
	//Started
	Log("-Sending video\n");

	//Mientras tengamos que capturar
	while(sendingVideo)
	{
		//Nos quedamos con el puntero antes de que lo cambien
		BYTE *pic = videoInput->GrabFrame();

		//Check picture
		if (!pic)
			//Exit
			continue;

		//Check if we need to send intra
		if (sendFPU)
		{
			//Set it
			videoEncoder->FastPictureUpdate();
			//Do not send anymore
			sendFPU = false;
		}
		//Procesamos el frame
		VideoFrame *videoFrame = videoEncoder->EncodeFrame(pic,videoInput->GetBufferSize());

		//If was failed
		if (!videoFrame)
			//Next
			continue;

		//Check
		if (frameTime)
		{
			timespec ts;
			//Lock
			pthread_mutex_lock(&mutex);
			//Calculate timeout
			calcAbsTimeout(&ts,&prev,frameTime);
			//Wait next or stopped
			int canceled  = !pthread_cond_timedwait(&cond,&mutex,&ts);
			//Unlock
			pthread_mutex_unlock(&mutex);
			//Check if we have been canceled
			if (canceled)
				//Exit
				break;
		}
		
		//Set next one
		frameTime = 1000/videoFPS;

		//Set frame timestamp
		videoFrame->SetTimestamp(getDifTime(&first)/1000);

		//Set sending time of previous frame
		getUpdDifTime(&prev);
		
		//Send it smoothly
		Smoothers::iterator iter;
	
		for(iter = smoothers.begin(); iter != smoothers.end(); iter++)
		{
			RTPSmoother* smoother = iter->second;
			smoother->SendFrame(videoFrame,frameTime);
		}
		
	}

	Log("-SendVideo out of loop\n");

	//Terminamos de capturar
	videoInput->StopVideoCapture();

	//Borramos el encoder
	delete videoEncoder;

	//Salimos
	Log("<SendVideo [%d]\n",sendingVideo);

	return 0;
}

/****************************
* CreateVideoEncoder
*	Crea un codificador de video para el tipo indicado
****************************/
VideoEncoder* GroupVideoStream::CreateVideoEncoder(VideoCodec::Type type)
{
	VideoEncoder *encoder = NULL;

	Log("-CreateVideoEncoder [%s,%d,%d]\n",VideoCodec::GetNameFor(type),videoQuality,videoFillLevel);

	//dependiendo del tipo
	switch(type)
	{
		case VideoCodec::MPEG4:
			encoder = (VideoEncoder*) new Mpeg4Encoder(1,31);
			break;
		case VideoCodec::H263_1998:
			encoder = (VideoEncoder*) new H263Encoder();
			break;
		case VideoCodec::H263_1996:
			encoder = (VideoEncoder*) new H263Encoder();
			break;
		case VideoCodec::H264:
			encoder = (VideoEncoder*) new H264Encoder();
			break;
		default:
			Log("Tipo de video no soportado\n");
	}

	return encoder;
}

void GroupVideoStream::SetPublishListener(PublishListener* listener)
{
	//Store listener
	this->listener = listener;
}


int GroupVideoStream::SendFPU()
{
	//Next shall be an intra
	sendFPU = true;
	
	return 1;
}

MediaStatistics GroupVideoStream::GetStatistics()
{
	MediaStatistics stats;

	//Fill stats
	//stats.isReceiving	= IsReceiving();
	stats.isSending		= IsSending();
	//stats.lostRecvPackets   = rtp.GetLostRecvPackets();
	//stats.numRecvPackets	= rtp.GetNumRecvPackets();
	//stats.numSendPackets	= rtp.GetNumSendPackets();
	//stats.totalRecvBytes	= rtp.GetTotalRecvBytes();
	//stats.totalSendBytes	= rtp.GetTotalSendBytes();

	//Return it
	return stats;
}
