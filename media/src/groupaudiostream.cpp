#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include "log.h"
#include "tools.h"
#include "audio.h"
#include "g711/g711codec.h"
#include "gsm/gsmcodec.h"
#include "groupaudiostream.h"


/**********************************
* AudioStream
*	Constructor
***********************************/
GroupAudioStream::GroupAudioStream()
{
	sendingAudio=0;
	recvingAudio=0;
	recvAudioThread = 0;
	sendAudioThread = 0;
	audioCodec=AudioCodec::PCMU;
	inited = false;
//	pthread_mutex_init(&rtcp_mutex, NULL);
}

/*******************************
* ~AudioStream
*	Destructor. 
********************************/
GroupAudioStream::~GroupAudioStream()
{
//	pthread_mutex_destroy(&rtcp_mutex);
}

/***************************************
* CreateAudioCodec
* 	Crea un objeto de codec de audio del tipo correspondiente
****************************************/
AudioCodec* GroupAudioStream::CreateAudioCodec(AudioCodec::Type codec)
{

	Log("-CreateAudioCodec [%d]\n",codec);

	//Creamos uno dependiendo del tipo
	switch(codec)
	{
		case AudioCodec::GSM:
			return new GSMCodec();
		case AudioCodec::PCMA:
			return new PCMACodec();
		case AudioCodec::PCMU:
			return new PCMUCodec();
		default:
			Log("Codec de audio erroneo [%d]\n",codec);
	}
	
	return NULL;
}

/***************************************
* SetAudioCodec
*	Fija el codec de audio
***************************************/
int GroupAudioStream::SetAudioCodec(AudioCodec::Type codec)
{
	//Compromabos que soportamos el modo
	if (!(codec==AudioCodec::PCMA || codec==AudioCodec::GSM || codec==AudioCodec::PCMU))
		return 0;

	//Colocamos el tipo de audio
	audioCodec = codec;

	Log("-SetAudioCodec [%d,%s]\n",audioCodec,AudioCodec::GetNameFor(audioCodec));

	inited = true;

	//Y salimos
	return 1;	
}

/***************************************
* Init
*	Inicializa los devices 
***************************************/
int GroupAudioStream::Init(AudioInput *input, AudioOutput *output)
{
	Log(">Init audio stream\n");


	//Nos quedamos con los puntericos
	audioInput  = input;
	audioOutput = output;

	//Y aun no estamos mandando nada
	sendingAudio=0;
	recvingAudio=0;

	Log("<Init audio stream end\n");

	return 1;
}

int GroupAudioStream::RecvAudioRtcp()
{
	/*
	BYTE	data[MTU];
	DWORD		size;
	Log(">RecvAudioRtcp\n");
	while(recvingAudio)
	{
		pthread_mutex_lock(&rtcp_mutex);
		Rtps::iterator iter;
		for(iter = rtps.begin(); iter != rtps.end(); iter++)
		{
			RTPSession* rtp = iter->second;
			if(rtp && rtp->IsFromRtsp() && rtp->GetRtcpPacket(data, &size)) {
				int sr_size = rtp->SendRtcpSRPacket();
			}
		}
		pthread_mutex_unlock(&rtcp_mutex);
	}
	Log("<RecvAudioRtcp end\n");
	*/
	pthread_exit(0);
}

/***************************************
* startSendingAudio
*	Helper function
***************************************/
void * GroupAudioStream::startRecvingAudioRtcp(void *par)
{
	GroupAudioStream *conf = (GroupAudioStream *)par;
	blocksignals();
	Log("RecvAudioRtcpThread [%d]\n",getpid());
	pthread_exit((void *)conf->RecvAudioRtcp());
}


RTPSession *GroupAudioStream::StartReceivingRtcp(unsigned clientSessionId)
{
	Log(">StartReceivingRtcp Group audio [%u]\n",clientSessionId);

	Rtps::iterator it = rtps.find(clientSessionId);
	RTPSession *rtp = NULL;

	if (it != rtps.end()) {
		Error("client already opened\n");
		return NULL;
	}

	rtp = new RTPSession(NULL);
	if(!rtp->Init())
	{
		delete rtp;
		return NULL;
	}
	rtp->FromRtsp();

	rtps[clientSessionId] = rtp;

	/*
	pthread_mutex_lock(&rtcp_mutex);
	rtcps[clientSessionId] = rtp;
	pthread_mutex_unlock(&rtcp_mutex);

	if(recvingAudio == 0)
	{
		//Arrancamos el thread de envio
		recvingAudio=1;
		//Start thread
		createPriorityThread(&recvAudioThread,startRecvingAudioRtcp,this,1);
	}
	*/
	Log("<StartReceivingRtcp audio [%u]\n",clientSessionId);

	return rtp;
}

/***************************************
* startSendingAudio
*	Helper function
***************************************/
void * GroupAudioStream::startSendingAudio(void *par)
{
	GroupAudioStream *conf = (GroupAudioStream *)par;
	blocksignals();
	Log("SendAudioThread [%d]\n",getpid());
	pthread_exit((void *)conf->SendAudio());
}

/***************************************
* StartSending
*	Comienza a mandar a la ip y puertos especificados
***************************************/
RTPSession *GroupAudioStream::StartSending(unsigned clientSessionId,char *sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap, RTPSession *rtp)
{
	Log(">StartSending Group audio [%u,%s,%d]\n",clientSessionId, sendAudioIp,sendAudioPort);

	//Si tenemos audio
	if (sendAudioPort==0) {
		//Error
		Error("Audio port 0\n");
		return NULL;
	}

	Rtps::iterator it = rtps.find(clientSessionId);
	
	if (it != rtps.end())
		rtp = (*it).second;

	if(rtp == NULL)
	{
		rtp = new RTPSession(NULL);
		if(!rtp->Init())
		{
			delete rtp;
			Error("Rtp init error\n");
			return NULL;
		}
	}

	
	//Iniciamos las sesiones rtp de envio
	if(!rtp->SetRemotePort(sendAudioIp,sendAudioPort))
	{
		delete rtp;
		Error("Rtp SetRemotePort error\n");
		return NULL;
	}

	//Set sending map
	rtp->SetSendingAudioRTPMap(rtpMap);

	//Set audio codec
	if(!rtp->SetSendingAudioCodec(audioCodec))
	{
		//Error
		delete rtp;
		Error("%s audio codec not supported by peer\n",AudioCodec::GetNameFor(audioCodec));
		return NULL;
	}

	rtps[clientSessionId] = rtp;

	if(sendingAudio == 0)
	{
		//Arrancamos el thread de envio
		sendingAudio=1;
		//Start thread
		createPriorityThread(&sendAudioThread,startSendingAudio,this,1);
	}

	/*
	pthread_mutex_lock(&rtcp_mutex);
	if(rtcps[clientSessionId] == NULL)
		rtcps[clientSessionId] = rtp;
	pthread_mutex_unlock(&rtcp_mutex);

	if(recvingAudio == 0)
	{
		//Arrancamos el thread de envio
		recvingAudio=1;
		//Start thread
		createPriorityThread(&recvAudioThread,startRecvingAudioRtcp,this,1);
	}
	*/

	Log("<StartSending audio [%u]\n",clientSessionId);

	return rtp;
}

/***************************************
* End
*	Termina la conferencia activa
***************************************/
int GroupAudioStream::End()
{
	Log(">End\n");

	if (sendingAudio)
	{
		while(rtps.size()>0)
			StopSending(rtps.begin()->first);
	}

	Log("<GroupAudioStream End\n");

	return 1;
}

/***************************************
* StopSending
* 	Termina el envio
****************************************/
int GroupAudioStream::StopSending(unsigned clientSessionId)
{
	Log(">StopSending Group Audio [%u]\n", clientSessionId);
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

	//El iterator
	Rtps::iterator it = rtps.find(clientSessionId);

	//Si no esta
	if (it == rtps.end())
		return Error("Rtps not found\n");
	
	RTPSession* rtp = (*it).second;
	
	rtps.erase(it);
	
	rtp->End();

	// delete it because it create from rtsp session
	if(rtp->IsFromRtsp())
		delete rtp;

	//Esperamos a que se cierren las threads de envio
	if(rtps.size() == 0)
	{
		//paramos
		sendingAudio=0;
		//Y esperamos
		pthread_join(sendAudioThread,NULL);
		sendAudioThread = 0;
	}
	
	/*
	if(rtcps.size() == 0)
	{
		recvingAudio=0;
		if(recvAudioThread!=0) {
			pthread_join(recvAudioThread,NULL);
			recvAudioThread = 0;
		}
	}
	*/

	Log("<StopSending Audio [%u]\n", clientSessionId);

	return 1;	
}

/*******************************************
* SendAudio
*	Capturamos el audio y lo mandamos
*******************************************/
int GroupAudioStream::SendAudio()
{
	WORD 		recBuffer[512];
	BYTE		data[512];
        int 		sendBytes=0;
        struct timeval 	before;
	AudioCodec* 	codec;
	DWORD		frameTime=0;

	Log(">SendGroupAudio\n");

	//Obtenemos el tiempo ahora
	gettimeofday(&before,NULL);

	//Creamos el codec de audio
	if ((codec = CreateAudioCodec(audioCodec))==NULL)
	{
		Log("Error en el envio de audio,saliendo\n");
		return 0;
	}

	//Empezamos a grabar
	audioInput->StartRecording();

	//Mientras tengamos que capturar
	while(sendingAudio)
	{
		//Incrementamos el tiempo de envio
		frameTime += codec->numFrameSamples;

		//Capturamos 
		if (audioInput->RecBuffer((WORD *)recBuffer,codec->numFrameSamples)==0)
		{
			Log("-sendingAudio cont\n");
			continue;
		}

		//Analizamos el paquete para ver si es de ceros
		int silence = true;

		//Recorremos el paquete
		for (int i=0;i<codec->numFrameSamples;i++)
			//Hasta encontrar uno que no sea cero
			if (recBuffer[i]!=0)
			{
				//No es silencio
				silence = false;
				//Salimos
				break;
			}

		//Si es silencio pasamos al siguiente
		if (silence)
			continue;
	
		//Lo codificamos
		int len = codec->Encode(recBuffer,codec->numFrameSamples,data,512);

		//Comprobamos que ha sido correcto
		if(len<=0)
		{
			Log("Error codificando el packete de audio\n");
			continue;
		}

		//Send it smoothly
		Rtps::iterator iter;

		for(iter = rtps.begin(); iter != rtps.end(); iter++)
		{
			RTPSession* rtp = iter->second;
			//Lo enviamos
			if(!rtp->SendAudioPacket((BYTE *)data,len,frameTime))
			{
				Log("Error mandando el packete de audio\n");
				continue;
			}
		}

		//Y reseteamos el frameTime
		frameTime = 0;

		//Aumentamos lo enviado
		sendBytes+=len;
	}

	Log("-SendAudio cleanup[%d]\n",sendingAudio);

	//Paramos de grabar por si acaso
	audioInput->StopRecording();

	//Logeamos
	Log("-Deleting codec\n");

	//Borramos el codec
	delete codec;

	//Salimos
        Log("<SendAudio\n");
	pthread_exit(0);
}

MediaStatistics GroupAudioStream::GetStatistics()
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
