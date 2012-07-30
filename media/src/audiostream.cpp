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
//#include "AudioMediaSubsession.hh"
//#include "live555MediaServer.h"
#include "rtpparticipant.h"
#include "audio.h"
#include "g711/g711codec.h"
#include "gsm/gsmcodec.h"
#include "audiostream.h"


/**********************************
* AudioStream
*	Constructor
***********************************/
AudioStream::AudioStream(RTPSession::Listener* listener) : rtp(listener)
{
	sendingAudio=0;
	receivingAudio=0;
	audioCodec=AudioCodec::PCMU;
	// add for rtsp watcher by liuhong start
	m_Participant = NULL;
	//audioSubsession = NULL;
	decodeCodec=AudioCodec::PCMU;
	muted = false;
	// add for rtsp watcher by liuhong end
}

/*******************************
* ~AudioStream
*	Destructor. 
********************************/
AudioStream::~AudioStream()
{
}

/***************************************
* CreateAudioCodec
* 	Crea un objeto de codec de audio del tipo correspondiente
****************************************/
AudioCodec* AudioStream::CreateAudioCodec(AudioCodec::Type codec)
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
int AudioStream::SetAudioCodec(AudioCodec::Type codec)
{
	//Compromabos que soportamos el modo
	if (!(codec==AudioCodec::PCMA || codec==AudioCodec::GSM || codec==AudioCodec::PCMU))
		return 0;

	//Colocamos el tipo de audio
	audioCodec = codec;

	Log("-SetAudioCodec [%d,%s]\n",audioCodec,AudioCodec::GetNameFor(audioCodec));

	//Y salimos
	return 1;	
}

/***************************************
* Init
*	Inicializa los devices 
***************************************/
int AudioStream::Init(AudioInput *input, AudioOutput *output)
{
	Log(">Init audio stream\n");

	//Iniciamos el rtp
	if(!rtp.Init())
		return Error("No hemos podido abrir el rtp\n");
	

	//Nos quedamos con los puntericos
	audioInput  = input;
	audioOutput = output;

	//Y aun no estamos mandando nada
	sendingAudio=0;
	receivingAudio=0;

	Log("<Init audio stream\n");

	return 1;
}

/***************************************
* startSendingAudio
*	Helper function
***************************************/
void * AudioStream::startSendingAudio(void *par)
{
	AudioStream *conf = (AudioStream *)par;
	blocksignals();
	Log("SendAudioThread [%d]\n",getpid());
	pthread_exit((void *)conf->SendAudio());
}

/***************************************
* startReceivingAudio
*	Helper function
***************************************/
void * AudioStream::startReceivingAudio(void *par)
{
	AudioStream *conf = (AudioStream *)par;
	blocksignals();
	Log("RecvAudioThread [%d]\n",getpid());
	pthread_exit((void *)conf->RecAudio());
}

/***************************************
* StartSending
*	Comienza a mandar a la ip y puertos especificados
***************************************/
int AudioStream::StartSending(char *sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap)
{
	Log(">StartSending audio [%s,%d]\n",sendAudioIp,sendAudioPort);

	//Si estabamos mandando tenemos que parar
	if (sendingAudio)
		//paramos
		StopSending();
	
	//Si tenemos audio
	if (sendAudioPort==0)
		//Error
		return Error("Audio port 0\n");


	//Y la de audio
	if(!rtp.SetRemotePort(sendAudioIp,sendAudioPort))
		//Error
		return Error("Error en el SetRemotePort\n");

	//Set sending map
	rtp.SetSendingAudioRTPMap(rtpMap);

	//Set audio codec
	if(!rtp.SetSendingAudioCodec(audioCodec))
		//Error
		return Error("%s audio codec not supported by peer\n",AudioCodec::GetNameFor(audioCodec));

	//Arrancamos el thread de envio
	sendingAudio=1;

	//Start thread
	createPriorityThread(&sendAudioThread,startSendingAudio,this,1);

	Log("<StartSending audio [%d]\n",sendingAudio);

	return 1;
}

/***************************************
* StartReceiving
*	Abre los sockets y empieza la recetpcion
****************************************/
int AudioStream::StartReceiving(AudioCodec::RTPMap& rtpMap)
{
	//If already receiving
	if (receivingAudio)
		//Stop it
		StopReceiving();

	//Get local rtp port
	int recAudioPort = rtp.GetLocalPort();

	//Set receving map
	rtp.SetReceivingAudioRTPMap(rtpMap);

	//We are reciving audio
	receivingAudio=1;

	//Create thread
	createPriorityThread(&recAudioThread,startReceivingAudio,this,1);

	//Log
	Log("<StartReceiving audio [%d]\n",recAudioPort);

	//Return receiving port
	return recAudioPort;
}

/***************************************
* End
*	Termina la conferencia activa
***************************************/
int AudioStream::End()
{
	//Terminamos de enviar
	StopSending();

	//Y de recivir
	StopReceiving();

	// add for rtsp watcher by liuhong start
	m_Participant = NULL;
	//if(audioSubsession != NULL)
	//	audioSubsession->End();
	// add for rtsp watcher by liuhong end

	//Cerramos la session de rtp
	rtp.End();

	return 1;
}

/***************************************
* StopReceiving
* 	Termina la recepcion
****************************************/

int AudioStream::StopReceiving()
{
	Log(">StopReceiving Audio\n");

	//Y esperamos a que se cierren las threads de recepcion
	if (receivingAudio)
	{	
		//Paramos de enviar
		receivingAudio=0;

		//Y unimos
		pthread_join(recAudioThread,NULL);
	}

	// add for rtsp watcher by liuhong start
	while(rtps.size()>0)
			DeleteAudioWatcher(rtps.begin()->first);
	// add for rtsp watcher by liuhong end

	Log("<StopReceiving Audio\n");

	return 1;

}

/***************************************
* StopSending
* 	Termina el envio
****************************************/
int AudioStream::StopSending()
{
	Log(">StopSending Audio\n");

	//Esperamos a que se cierren las threads de envio
	if (sendingAudio)
	{
		//paramos
		sendingAudio=0;

		//Cancel
		audioInput->CancelRecBuffer();

		//Y esperamos
		pthread_join(sendAudioThread,NULL);
	}

	Log("<StopSending Audio\n");

	return 1;	
}


/****************************************
* RecAudio
*	Obtiene los packetes y los muestra
*****************************************/
int AudioStream::RecAudio()
{
	BYTE 		lost=0;
	int 		recBytes=0;
	struct timeval 	before;
	BYTE		data[512];
	WORD		playBuffer[512];
	DWORD		size=512;
	AudioCodec*	codec=NULL;
	AudioCodec::Type type;
	DWORD		timeStamp=0;
	DWORD		frameTime=0;
	DWORD		lastTime=0;
	
	Log(">RecAudio\n");
	
	//Inicializamos el tiempo
	gettimeofday(&before,NULL);

	//Empezamos a reproducir
	audioOutput->StartPlaying();

	//Mientras tengamos que capturar
	while(receivingAudio)
	{
		//POnemos el tam�o
		size=512;
		lost=0;

		//Obtenemos el paquete
		if (!rtp.GetAudioPacket((BYTE *)data,&size,&lost,&type,&timeStamp))
		{
			//Log(">Process audio error\n");
			switch (errno)
			{
				case 11: case 4: case 0:
					break;
				default:
					Log("Error obteniendo el packete [%d]\n",errno);
			}
			continue;
		}
		
		// add for rtsp watcher by liuhong start
		if (!muted) {
			Rtps::iterator iter;
			for(iter = rtps.begin(); iter != rtps.end(); iter++)
			{
				RTPSession *rtp = iter->second;
				int ret = rtp->ForwardPacket((BYTE *)data, size, 0, timeStamp);
				//Log(">Send audio to watcher %d %ld\n", ret, timeStamp);
			}
		}

		//Comprobamos el tipo
		if ((codec==NULL) || (type!=codec->type))
		{
			//Si habia uno nos lo cargamos
			if (codec!=NULL)
				delete codec;

			//Creamos uno dependiendo del tipo
			if ((codec = CreateAudioCodec(type))==NULL)
			{
				Log("Error creando nuevo codec de audio [%d]\n",type);
				continue;
			}
			// add for rtsp watcher by liuhong
			decodeCodec = type;
		}
				
		//Lo decodificamos
		int len = codec->Decode(data,size,playBuffer,512);

		//Obtenemos el tiempo del frame
		frameTime = timeStamp - lastTime;

		//Actualizamos el ultimo envio
		lastTime = timeStamp;

		//Check muted
		if (!muted)
			//Y lo reproducimos
			audioOutput->PlayBuffer((WORD *)playBuffer,len,frameTime);

		//Aumentamos el numero de bytes recividos
		recBytes+=size;
	}

	//Terminamos de reproducir
	audioOutput->StopPlaying();

	//Borramos el codec
	if (codec!=NULL)
		delete codec;

	//Salimos
	Log("<RecAudio\n");
	pthread_exit(0);
}

/*******************************************
* SendAudio
*	Capturamos el audio y lo mandamos
*******************************************/
int AudioStream::SendAudio()
{
	WORD 		recBuffer[512];
	BYTE		data[512];
        int 		sendBytes=0;
        struct timeval 	before;
	AudioCodec* 	codec;
	DWORD		frameTime=0;

	Log(">SendAudio\n");

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

		//Lo enviamos
		if(!rtp.SendAudioPacket((BYTE *)data,len,frameTime))
		{
			Log("Error mandando el packete de audio\n");
			continue;
		}
//		Log("Send audio len %d\n",len);
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

MediaStatistics AudioStream::GetStatistics()
{
	MediaStatistics stats;

	//Fill stats
	stats.isReceiving	= IsReceiving();
	stats.isSending		= IsSending();
	stats.lostRecvPackets   = rtp.GetLostRecvPackets();
	stats.numRecvPackets	= rtp.GetNumRecvPackets();
	stats.numSendPackets	= rtp.GetNumSendPackets();
	stats.totalRecvBytes	= rtp.GetTotalRecvBytes();
	stats.totalSendBytes	= rtp.GetTotalSendBytes();

	//Return it
	return stats;
}

int AudioStream::SetMute(bool isMuted)
{
	//Set it
	muted = isMuted;
	//Exit
	return 1;
}

// add for rtsp watcher by liuhong start

int AudioStream::SetParticipant(RTPParticipant *participant)
{
	this->m_Participant = participant;
	return 1;
}
/*
AudioMediaSubsession* AudioStream::CreateAudioSubsession(AudioCodec::Type audioCodec, int payloadType)
{
	Live555MediaServer* live555MediaServer = Live555MediaServer::Instance();
	this->audioSubsession = AudioMediaSubsession::createNew(*live555MediaServer->GetUsageEnvironment(), this, audioCodec, payloadType);
	return this->audioSubsession;
}
*/

RTPSession *AudioStream::InitAudioWatcher(unsigned clientSessionId)
{
	Log(">Init Audio Watcher[%u]\n",clientSessionId);
	RTPSession* rtp = new RTPSession(NULL);
	rtps[clientSessionId] = rtp;
	Log("<Init Audio Watcher success [%u]\n",clientSessionId);
	return rtp;
}

RTPSession *AudioStream::AddAudioWatcher(unsigned clientSessionId,char *sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap, AudioCodec::Type watcherCodec)
{
	Log(">Add Audio Watcher[%u, %s,%d]\n",clientSessionId, sendAudioIp,sendAudioPort);

	//Si tenemos video
	if (sendAudioPort==0) {
		Error("No video\n");
		return NULL;
	}
	
	Rtps::iterator it = rtps.find(clientSessionId);

	RTPSession* rtp = NULL;
	//Si no esta
	if (it == rtps.end()) {
		Error("Rtp not init\n");
		return NULL;
	}
	rtp = (*it).second;

	if(!rtp->Init())
	{
		delete rtp;
		Error("Rtp init error\n");
		return NULL;
	}
	
	
	//Iniciamos las sesiones rtp de envio
	if(!rtp->SetRemotePort(sendAudioIp,sendAudioPort))
	{
		delete rtp;
		Error("Rtp SetRemotePort error\n");
		return NULL;
	}
	
	rtp->SetSendingAudioRTPMap(rtpMap);
	
	//Set video codec
	if(!rtp->SetSendingAudioCodec(watcherCodec))
	{
		delete rtp;
		//Error
		Error("%s video codec not supported by peer\n",AudioCodec::GetNameFor(watcherCodec));
		return NULL;
	}

	rtps[clientSessionId] = rtp;
	
	//LOgeamos
	Log("<Add Audio Watcher success [%u]\n",clientSessionId);

	return rtp;
}

int AudioStream::DeleteAudioWatcher(unsigned clientSessionId)
{
	Log(">DeleteAudioWatcher [%u]\n",clientSessionId);

	//El iterator
	Rtps::iterator it = rtps.find(clientSessionId);

	//Si no esta
	if (it == rtps.end())
		return Error("Rtps not found\n");
	
	RTPSession* rtp = (*it).second;
	
	rtps.erase(it);

	rtp->End();

	delete rtp;

	Log(">DeleteAudioWatcher [%u] success\n",clientSessionId);

	return 1;
}

// add for rtsp watcher by liuhong end
