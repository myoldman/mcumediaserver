/**
* @file RtspWatcher.cpp
* @brief Rtsp Watcher for a mcu conference
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "log.h"
#include "audioencoder.h"
#include "RtspWatcher.hh"
#include "H264FrameSource.hh"
#include "WavFrameSource.hh"
#include "H264MediaSubsession.hh"
#include "WavMediaSubsession.hh"
#include "multiconf.h"

#include <BasicUsageEnvironment.hh>
#include "live555MediaServer.h"
#include <string>

RtspWatcher::RtspWatcher()
{
	//Not inited
	inited = 0;

	//Set default codecs
	audioCodec = AudioCodec::PCMA;
	videoCodec = VideoCodec::H264_RTSP;
	m_StreamName = "mcu";
	videoSubsession = NULL;
	audioSubsession = NULL;
	m_Conf = NULL;
}

RtspWatcher::~RtspWatcher()
{
	if (inited)
	{
		StopStreaming();
		End();
	}
}

int RtspWatcher::SetVideoCodec(VideoCodec::Type codec,int mode,int fps,int bitrate,int quality, int fillLevel,int intraPeriod)
{
	Log("-----SetVideoCodec for rtsp watcher [%s,%dfps,%dkbps,quality:%d,fillLevel%d,intra:%d]\n",VideoCodec::GetNameFor(codec),fps,bitrate,quality,fillLevel,intraPeriod);

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
		case VideoCodec::H264_RTSP:
			videoQuality=1;
			videoFillLevel=6;
			break;
		default:
			//Return codec
			return Error("Codec not found\n");
	}

	videoCodec=codec;
	videoBitrate=bitrate;

	if (quality>0)
		videoQuality = quality;

	if (fillLevel>0)
		videoFillLevel = fillLevel;

	if (intraPeriod>0)
		videoIntraPeriod = intraPeriod;

	//Get width and height
	videoGrabWidth = GetWidth(mode);
	videoGrabHeight = GetHeight(mode);
	//Check size
	if (!videoGrabWidth || !videoGrabHeight)
		//Error
		return Error("Unknown video mode\n");

	videoCaptureMode=mode;
	videoFPS=fps;
	return 1;
}


int RtspWatcher::SetAudioCodec(AudioCodec::Type codec)
{
	Log("-----SetAudioCodec for rtsp watcher [%s]\n",AudioCodec::GetNameFor(codec));

	switch(codec)
	{
		case AudioCodec::PCMA:
		case AudioCodec::PCMU:
			this->numFrameSamples = 160;
			break;

		default:
			//Return codec
			return Error("Codec not found\n");
	}
	fSamplingFrequency = 8000;
	fNumChannels = 1;
	fBitsPerSample = 16;
	audioCodec = codec;
	return 1;
}

int RtspWatcher::Init(AudioInput* audioInput,VideoInput *videoInput, const std::string& streamName, MultiConf* conf)
{
	if (inited)
		return 0;
	
	//Store inputs
	this->audioInput = audioInput;
	this->videoInput = videoInput;
	this->m_StreamName = streamName;
	this->m_Conf = conf;

	//We are inited
	inited = 1;
	return 1;

}

int RtspWatcher::End()
{
	if (!inited)
		return 0;

	this->audioInput = NULL;
	this->videoInput = NULL;
	this->m_Conf = NULL;
	//Not inited
	inited = 0;
	return 1;
}

/***************************************
* StartEncoding
*	Comienza a mandar a la ip y puertos especificados
***************************************/
int RtspWatcher::StartStreaming()
{
	if (!inited)
		return Error("-----RTSP watcher not inited can not start streaming.\n");

	Log("-----RTSP wathcer start streaming.\n");

	this->videoInput->StartVideoCapture(videoGrabWidth,videoGrabHeight,videoFPS);
	this->audioInput->StartRecording();

	Live555MediaServer* live555MediaServer = Live555MediaServer::Instance();
	ServerMediaSession* sms = live555MediaServer->GetRTSPServer()->lookupServerMediaSession(m_StreamName.c_str());
	if(sms != NULL)
	{
		Log("-----RTSP wathcer already exist start streaming success\n");
		return 1;
	}

	//Start capturing
	sms = ServerMediaSession::createNew(*live555MediaServer->GetUsageEnvironment(), m_StreamName.c_str(), 0, "Session from mcu wather");
	videoSubsession = H264MediaSubsession::createNew(*live555MediaServer->GetUsageEnvironment(), this);
	audioSubsession = WavMediaSubsession::createNew(*live555MediaServer->GetUsageEnvironment(), this);

        sms->addSubsession(videoSubsession);
	sms->addSubsession(audioSubsession);
	live555MediaServer->GetRTSPServer()->addServerMediaSession(sms);

	Log("-----RTSP wathcer start streaming success\n");

	return 1;
}

int RtspWatcher::StopStreaming()
{
	Log("-----Rtsp watcher stop streaming\n");

	//this->videoInput->StopVideoCapture();
	this->audioInput->StopRecording();

	Live555MediaServer* live555MediaServer = Live555MediaServer::Instance();

	ServerMediaSession* sms = live555MediaServer->GetRTSPServer()->lookupServerMediaSession(m_StreamName.c_str());
	if(sms != NULL) 
	{
		Log("-----Rtsp watcher removeServerMediaSession %s\n", sms->streamName());
		if(videoSubsession)
			videoSubsession->End();
		if(audioSubsession)
			audioSubsession->End();
		live555MediaServer->GetRTSPServer()->removeServerMediaSession(sms);
	}
	
	Log("-----Rtsp watcher stop streaming success\n");

	return 1;
}

RTPSession *RtspWatcher::StartSendingVideo(unsigned clientSessionId,char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap)
{
	if(m_Conf)
	{
		return m_Conf->StartSendingVideo(clientSessionId, sendVideoIp, sendVideoPort, rtpMap);
	}
	return NULL;
}

int RtspWatcher::StopSendingVideo(unsigned clientSessionId)
{
	if(m_Conf)
	{
		m_Conf->StopSendingVideo(clientSessionId);
	}
}

RTPSession *RtspWatcher::StartSendingAudio(unsigned clientSessionId,char* sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap)
{
	if(m_Conf)
	{
		return m_Conf->StartSendingAudio(clientSessionId, sendAudioIp, sendAudioPort, rtpMap);
	}
	return NULL;
}

int RtspWatcher::StopSendingAudio(unsigned clientSessionId)
{
	if(m_Conf)
	{
		m_Conf->StopSendingAudio(clientSessionId);
	}
}

RTPSession *RtspWatcher::StartRecvingVideoRtcp(unsigned clientSessionId)
{
	if(m_Conf)
	{
		return m_Conf->StartRecvingVideoRtcp(clientSessionId);
	}
	return NULL;
}
	

RTPSession *RtspWatcher::StartRecvingAudioRtcp(unsigned clientSessionId)
{
	if(m_Conf)
	{
		return m_Conf->StartRecvingAudioRtcp(clientSessionId);
	}
	return NULL;
}


