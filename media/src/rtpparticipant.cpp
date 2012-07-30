/* 
 * File:   rtpparticipant.cpp
 * Author: Sergio
 * 
 * Created on 19 de enero de 2012, 18:41
 */

#include "rtpparticipant.h"
#include "log.h"
#include "VideoMediaSubsession.hh"
#include "AudioMediaSubsession.hh"
#include "live555MediaServer.h"

RTPParticipant::RTPParticipant(int partId, int confId) : Participant(Participant::RTP, partId, confId), audio(NULL) , video(this), text(NULL)
{
	// add for rtsp watcher by liuhong start
	rtpVideoMapIn = NULL;
	rtpAudioMapIn = NULL;
	char buffer[20] = {0};
	sprintf(buffer, "%d", confId);
	std::string strConfId = std::string(buffer);
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", partId);
	std::string strPartId = std::string(buffer);

	m_StreamName =strConfId + strPartId;
	video.SetParticipant(this);
	audio.SetParticipant(this);
	// add for rtsp watcher by liuhong end
}

RTPParticipant::~RTPParticipant()
{
}

int RTPParticipant::SetVideoCodec(VideoCodec::Type codec,int mode,int fps,int bitrate,int quality,int fillLevel,int intraPeriod)
{
	//Set it
	return video.SetVideoCodec(codec,mode,fps,bitrate,quality,fillLevel,intraPeriod);
}

int RTPParticipant::SetAudioCodec(AudioCodec::Type codec)
{
	//Set it
	return audio.SetAudioCodec(codec);
}

int RTPParticipant::SetTextCodec(TextCodec::Type codec)
{
	//Set it
	return text.SetTextCodec(codec);
}

int RTPParticipant::SendVideoFPU()
{
	//Send it
	return video.SendFPU();
}

MediaStatistics RTPParticipant::GetStatistics(MediaFrame::Type type)
{
	//Depending on the type
	switch (type)
	{
		case MediaFrame::Audio:
			return audio.GetStatistics();
		case MediaFrame::Video:
			return video.GetStatistics();
		default:
			return text.GetStatistics();
	}
}

int RTPParticipant::End()
{
	//End all streams
	video.End();
	audio.End();
	text.End();
	
	// add for rtsp watcher by liuuhong start
	Log("-----Rtsp watcher stop streaming\n");
	Live555MediaServer* live555MediaServer = Live555MediaServer::Instance();
	ServerMediaSession* sms = live555MediaServer->GetRTSPServer()->lookupServerMediaSession(m_StreamName.c_str());
	if(sms != NULL) 
	{
		Log("-----Rtsp watcher removeServerMediaSession %s\n", sms->streamName());
		// it will delete all sub session, so this should be done last
		live555MediaServer->GetRTSPServer()->removeServerMediaSession(sms);
	}
	Log("-----Rtsp watcher stop streaming success\n");
	// add for rtsp watcher by liuuhong end
}

int RTPParticipant::Init()
{
	//Init each stream
	video.Init(videoInput,videoOutput);
	audio.Init(audioInput,audioOutput);
	text.Init(textInput,textOutput);
}

int RTPParticipant::StartSendingVideo(char *ip,int port,VideoCodec::RTPMap& rtpMap)
{
	//Start sending
	return video.StartSending(ip,port,rtpMap);
}

int RTPParticipant::StopSendingVideo()
{
	//Stop sending
	return video.StopSending();
}

int RTPParticipant::StartReceivingVideo(VideoCodec::RTPMap& rtpMap)
{
	// add for rtsp watcher by liuhong start
	//If we already have one
	if (rtpVideoMapIn)
	{
		delete(rtpVideoMapIn);
		rtpVideoMapIn = NULL;
	}
	//Clone it
	rtpVideoMapIn = new VideoCodec::RTPMap(rtpMap);
	// add for rtsp watcher by liuhong end

	//Start receiving
	return video.StartReceiving(rtpMap);
}

int RTPParticipant::StopReceivingVideo()
{
	// add for rtsp watcher by liuhong start
	//If we already have one
	if (rtpVideoMapIn)
	{
		delete(rtpVideoMapIn);
		rtpVideoMapIn = NULL;
	}
	// add for rtsp watcher by liuhong end

	//Stop receiving
	return video.StopReceiving();
}

int RTPParticipant::StartSendingAudio(char *ip,int port,AudioCodec::RTPMap& rtpMap)
{
	//Start sending
	return audio.StartSending(ip,port,rtpMap);
}

int RTPParticipant::StopSendingAudio()
{
	//Stop sending
	return audio.StopSending();
}

int RTPParticipant::StartReceivingAudio(AudioCodec::RTPMap& rtpMap)
{
	// add for rtsp watcher by liuhong start
	//If we already have one
	if (rtpAudioMapIn)
	{
		delete(rtpAudioMapIn);
		rtpAudioMapIn = NULL;
	}
	//Clone it
	rtpAudioMapIn = new AudioCodec::RTPMap(rtpMap);
	// add for rtsp watcher by liuhong end
	//Start receiving
	return audio.StartReceiving(rtpMap);
}

int RTPParticipant::StopReceivingAudio()
{
	// add for rtsp watcher by liuhong start
	// If we already have one
	if (rtpAudioMapIn)
	{
		delete(rtpAudioMapIn);
		rtpAudioMapIn = NULL;
	}
	// add for rtsp watcher by liuhong end

	//Stop receiving
	return audio.StopReceiving();
}

int RTPParticipant::StartSendingText(char *ip,int port,TextCodec::RTPMap& rtpMap)
{
	//Start sending
	return text.StartSending(ip,port,rtpMap);
}

int RTPParticipant::StopSendingText()
{
	//Stop sending
	return text.StopSending();

}

int RTPParticipant::StartReceivingText(TextCodec::RTPMap& rtpMap)
{
	//Start receiving
	return text.StartReceiving(rtpMap);
}

int RTPParticipant::StopReceivingText()
{
	//Stop receiving
	return text.StopReceiving();
}

void RTPParticipant::onFPURequested(RTPSession *session)
{
	//Request it
	video.SendFPU();
}

void RTPParticipant::onRequestFPU()
{
	//Check
	if (listener)
		//Call listener
		listener->onRequestFPU(this);
}
int RTPParticipant::SetMute(MediaFrame::Type media, bool isMuted)
{
	//Depending on the type
	switch (media)
	{
		case MediaFrame::Audio:
			// audio
			return audio.SetMute(isMuted);
		case MediaFrame::Video:
			//Attach audio
			return video.SetMute(isMuted);
		case MediaFrame::Text:
			//text
			return text.SetMute(isMuted);
	}
	return 0;
}

// add for rtsp watcher by liuhong start

void RTPParticipant::StartRtspWatcher(char *sps_base64, char *pps_base64)
{
	VideoCodec::Type videoCodec = video.GetDecodeCodec();
	AudioCodec::Type audioCodec = audio.GetDecodeCodec();
	VideoMediaSubsession *videoSubsession = NULL;
	AudioMediaSubsession *audioSubsession = NULL;
	int videoPayloadType = -1;
	int audioPayloadType = -1;

	//Try to find it in the map
	for (VideoCodec::RTPMap::iterator it = rtpVideoMapIn->begin(); it!=rtpVideoMapIn->end(); ++it)
	{
			//Is it ourr codec
		if (it->second==videoCodec)
		{
			videoPayloadType = it->first;
		}
	}
	
	if(rtpAudioMapIn != NULL)
	{
		for (AudioCodec::RTPMap::iterator it = rtpAudioMapIn->begin(); it!=rtpAudioMapIn->end(); ++it)
		{
				//Is it ourr codec
			if (it->second==audioCodec)
			{
				audioPayloadType = it->first;
			}
		}
	}

	Log("-----Participant RTSP wathcer start streaming.\n");
	Live555MediaServer* live555MediaServer = Live555MediaServer::Instance();
	ServerMediaSession* sms = live555MediaServer->GetRTSPServer()->lookupServerMediaSession(m_StreamName.c_str());
	if(sms == NULL)
	{
		sms = ServerMediaSession::createNew(*live555MediaServer->GetUsageEnvironment(), m_StreamName.c_str(), 0, "Session from mcu participant watcher");

		videoSubsession = video.CreateVideoSubsession(videoCodec, videoPayloadType, sps_base64, pps_base64);
		sms->addSubsession(videoSubsession);
		if(audioPayloadType >= 0)
		{
			AudioMediaSubsession *audioSubsession = audio.CreateAudioSubsession(audioCodec, audioPayloadType);
			sms->addSubsession(audioSubsession);
		}

		live555MediaServer->GetRTSPServer()->addServerMediaSession(sms);
		Log("-----Participant wathcer start streaming success\n");
	} else {
		Log("-----Participant RTSP wathcer already exist\n");
	}
}

// add for rtsp watcher by liuhong end
