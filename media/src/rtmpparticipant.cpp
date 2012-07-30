/* 
 * File:   rtmpparticipant.cpp
 * Author: Sergio
 * 
 * Created on 19 de enero de 2012, 18:43
 */
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include "log.h"
#include "rtmpparticipant.h"
#include "codecs.h"
#include "rtpsession.h"
#include "video.h"
#include "h263/h263codec.h"
#include "flv1/flv1codec.h"
#include "avcdescriptor.h"
#include "fifo.h"
#include <wchar.h>

RTMPParticipant::RTMPParticipant(int partId, int confId) :  Participant(Participant::RTMP, partId, confId), audioFrames(), videoFrames()
{
	//Neither sending nor receiving
	sendingAudio = false;
	sendingVideo = false;
	sendingText = false;
	receivingAudio = false;
	receivingVideo = false;
	sendFPU = false;
	//set mutes
	audioMuted = false;
	videoMuted = false;
	textMuted = false;
	//Neither transmitter nor receiver
	receiver = NULL;
	publisher = NULL;
	meta = NULL;
	inited = false;

	//Inicializamos los mutex
	pthread_mutex_init(&mutex,NULL);
}

RTMPParticipant::~RTMPParticipant()
{
	//End it just in case
	End();
	//Destroy mutex
	pthread_mutex_destroy(&mutex);
}

int RTMPParticipant::SetVideoCodec(VideoCodec::Type codec,int mode,int fps,int bitrate,int quality,int fillLevel,int intraPeriod)
{
	//LO guardamos
	videoCodec=codec;

	//Guardamos el bitrate
	videoBitrate=bitrate;

	//Store intra period
	videoIntraPeriod = intraPeriod;

	//Get width and height
	videoWidth = GetWidth(mode);
	videoHeight = GetHeight(mode);

	//Check size
	if (!videoWidth || !videoHeight)
		//Error
		return Error("Unknown video mode\n");

	//Y los fps
	videoFPS=fps;
}

int RTMPParticipant::SetAudioCodec(AudioCodec::Type codec)
{
	//Set it
	audioCodec = codec;
}

int RTMPParticipant::SetTextCodec(TextCodec::Type codec)
{
	//do nothing
	return true;
}

int RTMPParticipant::SendVideoFPU()
{
	//Flag it
	sendFPU = true;
	//Send it
	return true;
}

int RTMPParticipant::Init()
{
	//We are started
	inited = true;
	//Wait for first Iframe
	waitVideo = true;
	//Set first timestamp
	getUpdDifTime(&first);
	return true;
}

int RTMPParticipant::StartSending(RTMPStream* receiver)
{
	Log(">RTMPParticipant start sending\n");
	
	//Check it
	if (!inited)
		//Exit
		return Error("Not inited\n");

	//Check receiver
	if (!receiver)
		//Error
		return Error("Null receiver\n");

	//Check if we already have a receiver
	if (this->receiver)
		//error
		return Error("Already sending\n");

	//Store receiver
	this->receiver = receiver;

	//If we already have metadata
	if (meta)
		//Send it
		receiver->PlayMetaData(meta);

	//Start all
	StartSendingAudio();
	StartSendingVideo();
	StartSendingText();

	Log("<RTMPParticipant start sending\n");
}

int RTMPParticipant::StopSending()
{
	//Check receiver
	if (!receiver)
		//Error
		return Error("Not sending\n");
	
	//Lock
	pthread_mutex_lock(&mutex);

	//Stop everithing
	StopSendingAudio();
	StopSendingVideo();
	StopSendingText();

	//Store it
	RTMPStream *old = receiver;
	//No receiver
	receiver = NULL;
	//Check not null
	if (old)
		//Close old
		old->Close();

	//Lock
	pthread_mutex_unlock(&mutex);

	//Ok
	return 1;
}

int RTMPParticipant::StartReceiving(RTMPStream* publisher)
{
	Log(">RTMPParticipant start receiving\n");

	//Check it
	if (!inited)
		//Exit
		return Error("-StartReceiving without beeing inited\n");
	
	//check published
	if (!publisher)
		//Error
		return Error("-StarReceiving with null publisher\n");
	
	//Check if we already have a publisher
	if (this->publisher)
		//error
		return false;
	//Store it
	this->publisher = publisher;
	//Start all
	StartReceivingAudio();
	StartReceivingVideo();

	Log("<RTMPParticipant start receiving\n");
}

int RTMPParticipant::StopReceiving()
{
	//Check
	if (!publisher)
		//Error
		return Error("Not publisher\n");

	//Lock
	pthread_mutex_lock(&mutex);

	//Temporal store publisher
	RTMPStream *old = publisher;
	//No publisher
	publisher = NULL;
	//Check not null
	if (old)
	{
		//Close old
		old->Close();
		//Stop everithing
		StopReceivingAudio();
		StopReceivingVideo();
	}

	//Unlock
	pthread_mutex_unlock(&mutex);

	//Exit
	return 1;
}

int RTMPParticipant::End()
{
	//Check if we are running
	if (!inited)
		return false;

	//Stop sending
	StopSending();
	//And receiver
	StopReceiving();

	//Remove meta
	if (meta)
		//delete objet
		delete(meta);
	//No meta
	meta = NULL;

	//Stop
	inited=0;

	return true;
}

bool RTMPParticipant::OnPublishedCommand(DWORD streamId,const wchar_t* name,AMFData* obj)
{
	return false;
}

bool RTMPParticipant::OnPublishedFrame(DWORD streamId,RTMPMediaFrame* frame)
{
	//Depending on the type
	switch (frame->GetType())
	{
		case RTMPMediaFrame::Video:
			//Push it
			videoFrames.Add((RTMPVideoFrame*)(frame->Clone()));
			break;
		case RTMPMediaFrame::Audio:
			//Convert to audio and push
			audioFrames.Add((RTMPAudioFrame*)(frame->Clone()));
			break;
	}

	return true;
}

bool RTMPParticipant::OnPublishedMetaData(DWORD streamId,RTMPMetaData *publishedMetaData)
{
	//Cheeck
	if (!sendingText)
		//Exit
		return false;

	//Get name
	AMFString* name = (AMFString*)publishedMetaData->GetParams(0);

	//Check it the send command text
	if (name->GetWString().compare(L"onText")==0)
	{
		//Get string
		AMFString *str = (AMFString*)publishedMetaData->GetParams(1);

		//Log
		Log("Sending t140 data [\"%ls\"]\n",str->GetWChar());

		//Create frame
		TextFrame frame(getDifTime(&first)/1000,str->GetWString());
		//Send text
		textOutput->SendFrame(frame);
	}

	return true;
}

int  RTMPParticipant::StartSendingVideo()
{
	Log("-StartSendingVideo\n");

	//Si estabamos mandando tenemos que parar
	if (sendingVideo)
		//Y esperamos que salga
		StopSendingVideo();

	//Estamos mandando
	sendingVideo=1;

	//Arrancamos los procesos
	createPriorityThread(&sendVideoThread,startSendingVideo,this,0);

	return sendingVideo;
}

int  RTMPParticipant::StopSendingVideo()
{
	Log(">StopSendingVideo\n");

	//Y esperamos a que se cierren las threads de recepcion
	if (sendingVideo)
	{
		//Dejamos de recivir
		sendingVideo=0;

		//Cancel frame cpature
		videoInput->CancelGrabFrame();

		//Esperamos
		pthread_join(sendVideoThread,NULL);
	}

	Log("<StopSendingVideo\n");
}

int  RTMPParticipant::StartReceivingVideo()
{
	//Si estabamos reciviendo tenemos que parar
	if (receivingVideo)
		//Stop receiving
		StopReceivingVideo();

	//Reset video frames queue
	videoFrames.Reset();

	//Estamos recibiendo
	receivingVideo=1;

	//Arrancamos los procesos
	createPriorityThread(&recVideoThread,startReceivingVideo,this,0);

	//Logeamos
	Log("-StartReceivingVideo\n");

	return 1;
}

int  RTMPParticipant::StopReceivingVideo()
{
	Log(">StopReceivingVideo\n");

	//Y esperamos a que se cierren las threads de recepcion
	if (receivingVideo)
	{
		//Dejamos de recivir
		receivingVideo=0;

		//Cancel any pending wait
		videoFrames.Cancel();

		//Esperamos
		pthread_join(recVideoThread,NULL);
	}

	Log("<StopReceivingVideo\n");

	return 1;
}

int  RTMPParticipant::StartSendingAudio()
{
	Log("-StartSendingAudio\n");

	//Si estabamos mandando tenemos que parar
	if (sendingAudio)
		//Y esperamos que salga
		StopSendingAudio();

	//Estamos mandando
	sendingAudio=1;

	//Arrancamos los procesos
	createPriorityThread(&sendAudioThread,startSendingAudio,this,0);

	return sendingAudio;
}

int  RTMPParticipant::StopSendingAudio()
{
	Log(">StopSendingAudio\n");

	//Y esperamos a que se cierren las threads de recepcion
	if (sendingAudio)
	{
		//Dejamos de recivir
		sendingAudio=0;

		//Cancel grab audio
		audioInput->CancelRecBuffer();
		
		//Esperamos
		pthread_join(sendAudioThread,NULL);
	}

	Log("<StopSendingAudio\n");

	return sendingAudio;
}

int  RTMPParticipant::StartReceivingAudio()
{
	//Si estabamos reciviendo tenemos que parar
	if (receivingAudio)
		//Stop receiving audio
		StopReceivingAudio();

	//Estamos recibiendo
	receivingAudio=1;

	//Reset audio frames queue
	audioFrames.Reset();

	//Arrancamos los procesos
	createPriorityThread(&recAudioThread,startReceivingAudio,this,0);

	//Logeamos
	Log("-StartReceivingAudio\n");

	return receivingAudio;
}

int  RTMPParticipant::StopReceivingAudio()
{
	Log(">StopReceivingAudio\n");

	//Y esperamos a que se cierren las threads de recepcion
	if (receivingAudio)
	{
		//Dejamos de recivir
		receivingAudio=0;

		//Cancel any pending wait
		audioFrames.Cancel();

		//Esperamos
		pthread_join(recAudioThread,NULL);
	}

	Log("<StopReceivingAudio\n");

	return 1;
}

void* RTMPParticipant::startSendingText(void *par)
{
	Log("RecTextThread [%d]\n",getpid());

	//Obtenemos el objeto
	RTMPParticipant *sess = (RTMPParticipant *)par;

	//Bloqueamos las se�a�es
	blocksignals();

	//Y ejecutamos
	pthread_exit( (void *)sess->SendText());
}

int  RTMPParticipant::StartSendingText()
{
	Log("-StartSendingText\n");

	//Si estabamos mandando tenemos que parar
	if (sendingText)
		//Y esperamos que salga
		StopSendingText();

	
	//Estamos mandando
	sendingText=1;

	return sendingText;
}

int  RTMPParticipant::StopSendingText()
{
	Log(">StopSendingText\n");

	//Dejamos de recivir
	sendingText=0;

	Log("<StopSendingText\n");

	return 1;
}

/**************************************
* startReceivingVideo
*	Function helper for thread
**************************************/
void* RTMPParticipant::startReceivingVideo(void *par)
{
	Log("RecVideoThread [%d]\n",getpid());

	//Obtenemos el objeto
	RTMPParticipant *sess = (RTMPParticipant *)par;

	//Bloqueamos las se�a�es
	blocksignals();

	//Y ejecutamos
	pthread_exit( (void *)sess->RecVideo());
}

/**************************************
* startSendingVideo
*	Function helper for thread
**************************************/
void* RTMPParticipant::startSendingVideo(void *par)
{
	Log("SendVideoThread [%d]\n",getpid());

	//Obtenemos el objeto
	RTMPParticipant *sess = (RTMPParticipant *)par;

	//Bloqueamos las se�a�es
	blocksignals();

	//Y ejecutamos
	pthread_exit( (void *)sess->SendVideo());
}

/**************************************
* startReceivingAudio
*	Function helper for thread
**************************************/
void* RTMPParticipant::startReceivingAudio(void *par)
{
	Log("RecVideoThread [%d]\n",getpid());

	//Obtenemos el objeto
	RTMPParticipant *sess = (RTMPParticipant *)par;

	//Bloqueamos las se�a�es
	blocksignals();

	//Y ejecutamos
	pthread_exit( (void *)sess->RecAudio());
}


/**************************************
* startSendingAudio
*	Function helper for thread
**************************************/
void* RTMPParticipant::startSendingAudio(void *par)
{
	Log("SendAudioThread [%d]\n",getpid());

	//Obtenemos el objeto
	RTMPParticipant *sess = (RTMPParticipant *)par;

	//Bloqueamos las se�a�es
	blocksignals();

	//Y ejecutamos
	pthread_exit( (void *)sess->SendAudio());
}

int RTMPParticipant::SendVideo()
{
	//Coders
	VideoEncoder* encoder = VideoCodecFactory::CreateEncoder(videoCodec);
	//Create new video frame
	RTMPVideoFrame  frame(0,65500);

	Log(">SendVideo\n");
	
	//Set codec
	switch(videoCodec)
	{
		case VideoCodec::SORENSON:
			//Set codec
			frame.SetVideoCodec(RTMPVideoFrame::FLV1);
			break;
		case VideoCodec::H264:
			//Set codec
			frame.SetVideoCodec(RTMPVideoFrame::AVC);
			//Set NALU type
			frame.SetAVCType(1);
			//Set no delay
			frame.SetAVCTS(0);
			break;
		default:
			return Error("Codec not supported\n");
	}

	//Set sice
	videoInput->StartVideoCapture(videoWidth,videoHeight,videoFPS);
	//Set size
	encoder->SetSize(videoWidth,videoHeight);

	//Set bitrate
	encoder->SetFrameRate(videoFPS,videoHeight,videoIntraPeriod);

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
			encoder->FastPictureUpdate();
			//Do not send anymore
			sendFPU = false;
		}

		//Encode next frame
		VideoFrame *encoded = encoder->EncodeFrame(pic,videoInput->GetBufferSize());

		//Check
		if (!encoded)
			break;

		//Check size
		if (frame.GetMaxMediaSize()<encoded->GetLength())
			//Not enougth space
			return Error("Not enought space to copy FLV encodec frame [frame:%d,encoded:%d",frame.GetMaxMediaSize(),encoded->GetLength());

		//Get full frame
		frame.SetVideoFrame(encoded->GetData(),encoded->GetLength());

		//Set buffer size
		frame.SetMediaSize(encoded->GetLength());

		//Set timestamp
		frame.SetTimestamp(getDifTime(&first)/1000);
		
		//Check type
		if (encoded->IsIntra())
		{
			//Set type
			frame.SetFrameType(RTMPVideoFrame::INTRA);
			//Check if it is an AVC and not send the description
			if (frame.GetVideoCodec()==RTMPVideoFrame::AVC)
			{
				//Crete desc frame
				RTMPVideoFrame frameDesc(0,2048);
				//Send
				frameDesc.SetTimestamp(frame.GetTimestamp());
				//Set type
				frameDesc.SetVideoCodec(RTMPVideoFrame::AVC);
				//Set type
				frameDesc.SetFrameType(RTMPVideoFrame::INTRA);
				//Set NALU type
				frameDesc.SetAVCType(0);
				//Set no delay
				frameDesc.SetAVCTS(0);
				//Create description
				AVCDescriptor desc;
				//Set values
				desc.SetConfigurationVersion(1);
				desc.SetAVCProfileIndication(0x42);
				desc.SetProfileCompatibility(0x80);
				desc.SetAVCLevelIndication(0x0C);
				desc.SetNALUnitLength(3);
				//Get encoded data
				BYTE *data = encoded->GetData();
				//Get size
				DWORD size = encoded->GetLength();
				//Chop into NALs
				while(size>4)
				{
					//Get nal size
					DWORD nalSize =  get4(data,0);
					//Get NAL start
					BYTE *nal = data+4;
					//Depending on the type
					switch(nal[0] & 0xF)
					{
						case 8:
							//Append
							desc.AddPictureParameterSet(nal,nalSize);
							break;
						case 7:
							//Append
							desc.AddSequenceParameterSet(nal,nalSize);
							break;
					}
					//Skip it
					data+=4+nalSize;
					size-=4+nalSize;
				}
				
				//Serialize
				DWORD len = desc.Serialize(frameDesc.GetMediaData(),frameDesc.GetMaxMediaSize());
				
				//Set size
				frameDesc.SetMediaSize(len);

				if (receiver)
					//Send it
					receiver->PlayMediaFrame(&frameDesc);
			}
		} else {
			//Set type
			frame.SetFrameType(RTMPVideoFrame::INTER);
		}
		if (receiver)
			//Send it
			receiver->PlayMediaFrame(&frame);
	}

	//Stop capture
	videoInput->StopVideoCapture();
	
	//Check
	if (encoder)
		//Delete
		delete(encoder);

	Log("<SendVideo\n");

	//Salimos
	pthread_exit(0);
}

int RTMPParticipant::SendAudio()
{
	AudioCodec*	rtmpAudioEncoder = NULL;

	//Create new audio frame
	RTMPAudioFrame  *audio = new RTMPAudioFrame(0,MTU);
	
        struct timeval 	before;

	Log(">SendAudio\n");

	//Obtenemos el tiempo ahora
	gettimeofday(&before,NULL);

	//Creamos el codec de audio
	if ((rtmpAudioEncoder = AudioCodec::CreateCodec(AudioCodec::NELLY11))==NULL)
	{
		Log("Error en el envio de audio,saliendo\n");
		return 0;
	}

	//Empezamos a grabar
	audioInput->StartRecording();

	//Mientras tengamos que capturar
	while(sendingAudio)
	{
		WORD recBuffer[512];

		//Capturamos
		DWORD  recLen = audioInput->RecBuffer((WORD *)recBuffer,rtmpAudioEncoder->numFrameSamples);

		//Check len
		if (!recLen)
		{
			Log("-sendingAudio cont");
			continue;
		}

		//Rencode it
		DWORD len;

		while((len=rtmpAudioEncoder->Encode(recBuffer,recLen,audio->GetMediaData(),audio->GetMaxMediaSize()))>0)
		{
			//REset
			recLen = 0;

			//Set length
			audio->SetMediaSize(len);

			switch(rtmpAudioEncoder->type)
			{
				case AudioCodec::SPEEX16:
					//Set RTMP data
					audio->SetAudioCodec(RTMPAudioFrame::SPEEX);
					audio->SetSoundRate(RTMPAudioFrame::RATE11khz);
					audio->SetSamples16Bits(1);
					audio->SetStereo(0);
					break;
				case AudioCodec::NELLY8:
					//Set RTMP data
					audio->SetAudioCodec(RTMPAudioFrame::NELLY8khz);
					audio->SetSoundRate(RTMPAudioFrame::RATE11khz);
					audio->SetSamples16Bits(1);
					audio->SetStereo(0);
					break;
				case AudioCodec::NELLY11:
					//Set RTMP data
					audio->SetAudioCodec(RTMPAudioFrame::NELLY);
					audio->SetSoundRate(RTMPAudioFrame::RATE11khz);
					audio->SetSamples16Bits(1);
					audio->SetStereo(0);
					break;
			}

			//Set timestamp
			audio->SetTimestamp(getDifTime(&first)/1000);

			//Check receiver
			if (receiver)
				//Send packet
				receiver->PlayMediaFrame(audio);
		}
	}

	//Check
	if (audio)
		//Delete it
		delete(audio);

	Log("<SendAudio\n");

	//Salimos
	pthread_exit(0);
}

int RTMPParticipant::SendText()
{
	Log(">SendText\n");

	//Mientras tengamos que capturar
	while(sendingText)
	{
		//Text frame
		TextFrame *frame = NULL;

		//Get frame
		frame = textInput->GetFrame(0);

		//Create new timestamp associated to latest media time
		RTMPMetaData *meta = new RTMPMetaData(frame->GetTimeStamp());

		//Add text name
		meta->AddParam(new AMFString(L"onText"));
		//Set data
		meta->AddParam(new AMFString(frame->GetWChar()));

		//Debug
		Log("Got T140 frame\n");
		meta->Dump();

		//Check receiver
		if (receiver)
			//Send packet
			receiver->PlayMetaData(meta);

		//Delete frame
		delete(frame);
	}

	Log("<SendText\n");

	//Salimos
	pthread_exit(0);
}

int RTMPParticipant::RecVideo()
{
	VideoDecoder *decoder = NULL;
	DWORD width = 0;
	DWORD height = 0;
	int NALUnitLength = 0;

	Log(">RecVideo\n");

	//While sending video
	while (receivingVideo)
	{
		//Wait for next video
		if (!videoFrames.Wait(0))
			//Check again
			continue;

		//Get audio grame
		RTMPVideoFrame* video = videoFrames.Pop();
		//check
		if (!video)
			//Again
			continue;

		//Check type to find decoder
		switch(video->GetVideoCodec())
		{
			case RTMPVideoFrame::FLV1:
				//Check if there is no decoder of of different type
				if (!decoder || decoder->type!=VideoCodec::SORENSON)
				{
					//check decoder
					if (decoder)
						//Delet
						delete(decoder);
					//Create sorenson one
					decoder = VideoCodecFactory::CreateDecoder(VideoCodec::SORENSON);
				}
				break;
			case RTMPVideoFrame::AVC:
				//Check if there is no decoder of of different type
				if (!decoder || decoder->type!=VideoCodec::H264)
				{
					//check decoder
					if (decoder)
						//Delet
						delete(decoder);
					//Create sorenson one
					decoder = VideoCodecFactory::CreateDecoder(VideoCodec::H264);
				}
				break;
			default:
				//Not found, ignore
				continue;
		}

		//Check if it is AVC descriptor
		if (video->GetVideoCodec()==RTMPVideoFrame::AVC && video->GetAVCType()==0)
		{
			AVCDescriptor desc;

			//Parse it
			desc.Parse(video->GetMediaData(),video->GetMaxMediaSize());

			//Get nal
			NALUnitLength = desc.GetNALUnitLength()+1,

			//Dump it
			desc.Dump();

			//Decode SPS
			for (int i=0;i<desc.GetNumOfSequenceParameterSets();i++)
				//Decode NAL
				decoder->DecodePacket(desc.GetSequenceParameterSet(i),desc.GetSequenceParameterSetSize(i),0,0);
			//Decode PPS
			for (int i=0;i<desc.GetNumOfPictureParameterSets();i++)
				//Decode NAL
				decoder->DecodePacket(desc.GetPictureParameterSet(i),desc.GetPictureParameterSetSize(i),0,0);

			//Nothing more needded;
			continue;
		} else if (video->GetVideoCodec()==RTMPVideoFrame::AVC && video->GetAVCType()==1) {
			//Malloc
			BYTE *data = video->GetMediaData();
			//Get size
			DWORD size = video->GetMediaSize();
			//Chop into NALs
			while(size>NALUnitLength)
			{
				DWORD nalSize = 0;
				//Get size
				if (NALUnitLength==4)
					//Get size
					nalSize = get4(data,0);
				else if (NALUnitLength==3)
					//Get size
					nalSize = get3(data,0);
				else if (NALUnitLength==2)
					//Get size
					nalSize = get2(data,0);
				else if (NALUnitLength==1)
					//Get size
					nalSize = data[0];
				else
					//Skip
					continue;
				//Get NAL start
				BYTE *nal = data+NALUnitLength;
				//Skip it
				data+=NALUnitLength+nalSize;
				size-=NALUnitLength+nalSize;
				//Decode it
				decoder->DecodePacket(nal,nalSize,0,(size<NALUnitLength));
			}
		} else {
			//Decode frame
			if (!decoder->Decode(video->GetMediaData(),video->GetMediaSize()))
			{
				Error("decode packet error");
				//Next
				continue;
			}
		}

		//Check size
		if (decoder->GetWidth()!=width || decoder->GetHeight()!=height)
		{
			//Get dimension
			width = decoder->GetWidth();
			height = decoder->GetHeight();

			//Set them in the encoder
			videoOutput->SetVideoSize(width,height);
		}

		//If it is muted
		if (!videoMuted)
			//Send
			videoOutput->NextFrame(decoder->GetFrame());

		//Delete video frame
		delete(video);
	}

	Log("<RecVideo\n");

	return 1;
}

int RTMPParticipant::RecAudio()
{
	AudioCodec::Type rtmpAudioCodec;
	AudioCodec *rtmpAudioDecoder = NULL;
	
	Log(">RecAudio\n");

	//While sending audio
	while (receivingAudio)
	{
		//Wait for next audio
		if (!audioFrames.Wait(0))
			//Check again
			continue;

		//Get audio grame
		RTMPAudioFrame* audio = audioFrames.Pop();

		//Check audio frame
		if (!audio)
			//Next one
			continue;
		
		//Get codec type
		switch(audio->GetAudioCodec())
		{
			case RTMPAudioFrame::SPEEX:
				//Set codec
				rtmpAudioCodec = AudioCodec::SPEEX16;
				break;
			case RTMPAudioFrame::NELLY:
				//Set codec type
				rtmpAudioCodec = AudioCodec::NELLY11;
				break;
			default:
				continue;
		}

		WORD raw[512];
		DWORD rawSize = 512;
		DWORD rawLen = 0;

		//Check if we have a decoder
		if (!rtmpAudioDecoder || rtmpAudioDecoder->type!=rtmpAudioCodec)
		{
			//Check
			if (rtmpAudioDecoder)
				//Delete old one
				delete(rtmpAudioDecoder);
			//Create new one
			rtmpAudioDecoder = AudioCodec::CreateDecoder(rtmpAudioCodec);
		}

		//Get data
		BYTE *data = audio->GetMediaData();
		//Get size
		DWORD size = audio->GetMediaSize();
		//Decode it until no frame is found
		while ((rawLen = rtmpAudioDecoder->Decode(data,size,raw,rawSize))>0)
		{
			//Check size
			if (rawLen>0 && !audioMuted)
				//Enqeueue it
				audioOutput->PlayBuffer(raw,rawLen,0);
			//Remove size
			size = 0;
		}
		
		//Delete audio
		delete(audio);
	}

	Log("<RecAudio\n");

	return 1;
}

/************************************
 * MediaBridgeInputStream
 *
 ************************************/
RTMPParticipant::InputStream::InputStream(DWORD streamId,MultiConf *conf) : RTMPStream(streamId)
{
	//Store the broarcaster
	this->conf = conf;
	//Not publishing
	part = NULL;
}

RTMPParticipant::InputStream::~InputStream()
{
	//If we are still publishing
	if (part)
		//Close it
		Close();
}

bool RTMPParticipant::InputStream::Play(std::wstring& url)
{
	//Not supported
	return false;
}

bool RTMPParticipant::InputStream::Seek(DWORD time)
{
	//Send error
	SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Seek.Failed",L"error",L"Seek not supported"));
	return false;
}

bool RTMPParticipant::InputStream::Publish(std::wstring& url)
{
	//Parse url
	std::wstring token(url);

	//Find query string
	int pos = url.find(L"?");

	//Remove query string
	if (pos>0)
		//Remove it
		token.erase(pos,token.length());

	//Get participant
	part = (RTMPParticipant*) conf->ConsumeParticipantInputToken(token);

	//Check if we have one
	if (!part)
		//Pin not found
		return Error("No participant set for [pin:\"%ls\"]\n",token.c_str());

	//Start receiving
	part->StartReceiving(this);

	//Set trasmitter
	SetPublishListener(part);

	return true;
}

bool RTMPParticipant::InputStream::Close()
{
	//If no session
	if (!part)
		//Exit
		return false;

	//Set trasmitter
	SetPublishListener(NULL);

	//Stop Sending
	part->StopReceiving();

	//Unlink us from session
	part = NULL;

	return true;
}

/************************************
 * MediaBridgeOutputStream
 *
 ************************************/
RTMPParticipant::OutputStream::OutputStream(DWORD streamId, MultiConf *conf) : RTMPStream(streamId)
{
	//Store the broarcaster
	this->conf = conf;
	//Set first
	first=(QWORD)-1;
	//Not publishing
	part = NULL;
}

RTMPParticipant::OutputStream::~OutputStream()
{
	//If we have not been closed
	if (part)
		//Close
		Close();
}

bool RTMPParticipant::OutputStream::Seek(DWORD time)
{
	//Send error
	SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Seek.Failed",L"error",L"Seek not supported"));
	return false;
}

bool RTMPParticipant::OutputStream::Play(std::wstring& url)
{
	Log("-Playing broadcast session [url:\"%ls\"]\n",url.c_str());

	//Remove extra data from FMS
	if (url.find(L"*flv:")==0)
		//Erase it
		url.erase(0,5);
	else if (url.find(L"flv:")==0)
		//Erase it
		url.erase(0,4);

	//Get participant
	part = (RTMPParticipant*) conf->ConsumeParticipantOutputToken(url.c_str());

	//In the future parse queryString also to get token
	if(!part)
	{
		//Send error
		playListener->OnCommand(id,L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.StreamNotFound",L"error",L"Token not found"));
		//Broadcast not found
		return Error("No participant found for token [%ls]\n",url.c_str());
	}

	//Start stream
	playListener->OnStreamBegin(id);

	//Start sending
	part->StartSending(this);

	//Exit
	return true;
}

void RTMPParticipant::OutputStream::PlayMediaFrame(RTMPMediaFrame* frame)
{
	//Get timestamp
	QWORD ts = frame->GetTimestamp();

	//Check if it is not negative
	if (ts!=(QWORD)-1)
	{
		//Check if it is first
		if (first==(QWORD)-1)
		{
			Log("-Got first frame [%llu]\n",ts);
			//Store timestamp
			first = ts;
			//Everything ok
			playListener->OnCommand(id,L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.Reset",L"status",L"Playback started") );
			//Send play comand
			SendCommand(L"onStatus", new RTMPNetStatusEvent(L"NetStream.Play.Start",L"status",L"Playback started") );
		}

		//Modify timestamp
		frame->SetTimestamp(ts-first);
	}

	//Send frame
	playListener->OnPlayedFrame(id,frame);
}

bool RTMPParticipant::OutputStream::Publish(std::wstring& url)
{
	//Only output
	return false;
}

bool RTMPParticipant::OutputStream::Close()
{
	//If no session
	if (!part)
		//Exit
		return false;
	
	//Stop sending
	part->StopSending();
	
	//Unlink us
	part = NULL;

	//Everything ok
	return true;
}

MediaStatistics RTMPParticipant::GetStatistics(MediaFrame::Type type)
{
	//Depending on the type
	switch (type)
	{
		case MediaFrame::Audio:
			return audioStats;
		case MediaFrame::Video:
			return videoStats;
		default:
			return textStats;
	}
}

int RTMPParticipant::SetMute(MediaFrame::Type media, bool isMuted)
{
	//Depending on the type
	switch (media)
	{
		case MediaFrame::Audio:
			//Set mute
			audioMuted = isMuted;
			//Exit
			return 1;
		case MediaFrame::Video:
			//Set mute
			videoMuted = isMuted;
			//Exit
			return 1;
		case MediaFrame::Text:
			//Set mute
			textMuted = isMuted;
			//Exit
			return 1;
	}

	return 0;
}
