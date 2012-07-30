/**
* @file H264FrameSource.cpp
* @brief H264 Frame source for live555 rtsp subsession
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/
#include <WavFrameSource.hh>
#include "log.h"
#include "g711/g711codec.h"
#include "gsm/gsmcodec.h"

WavFrameSource::WavFrameSource(UsageEnvironment &env, RtspWatcher *rtspWatcher) : FramedSource(env)
{
	m_audioInput = rtspWatcher->GetAudioInput();
	switch(rtspWatcher->GetAudioCodec())
	{
		case AudioCodec::GSM:
			audioEncoder = new GSMCodec();
			break;
		case AudioCodec::PCMA:
			audioEncoder =  new PCMACodec();
			break;
		case AudioCodec::PCMU:
			audioEncoder =  new PCMUCodec();
			break;
		default:
			Log("Codec de audio erroneo [%d]\n",rtspWatcher->GetAudioCodec());
	}
	
	fSamplingFrequency = rtspWatcher->GetSamplingFrequency();
	fNumChannels = rtspWatcher->GetNumChannels();
	fBitsPerSample = rtspWatcher->GetBitsPerSample();
	fPlayTimePerSample = 1e6/(double)fSamplingFrequency;
	fNumFrameSamples = rtspWatcher->GetNumFrameSamples();

	Log("-----Rstp Wav frame source created\n");
}


WavFrameSource::~WavFrameSource()
{
	Log("-----Rstp Wav frame source destroy\n");
	if(audioEncoder) 
	{
		delete  audioEncoder;
	}
}

void WavFrameSource::End()
{
	if(m_audioInput)
	{
		m_audioInput = NULL;
	}
}

void WavFrameSource::doGetNextFrame()
{
	if(!m_audioInput)
		return;
	//Log("-----Rtsp wav frame source nextframe begin\n");
	WORD 		recBuffer[512];
	BYTE		data[512];
	unsigned bytesPerSample = (fNumChannels*fBitsPerSample)/8;
	
	if (m_audioInput->RecBuffer((WORD *)recBuffer,this->fNumFrameSamples)==0)
	{
		Log("-----Rtsp wav frame source nextframe continue");
		return;
	}
	int silence = true;

	for (int i=0;i<audioEncoder->numFrameSamples;i++)
			if (recBuffer[i]!=0)
			{
				silence = false;
				break;
			}

	if (silence)
	{
//		Log("-----Rtsp wav frame source nextframe continue");
//		return;
	}
	
	int len = audioEncoder->Encode(recBuffer,this->fNumFrameSamples,data,512);

	if(len<=0)
	{
		Log("-----Rtsp wav frame source nextframe continue\n");
		return;
	}
	
	fFrameSize = len;
	
	memmove(fTo, data, fFrameSize);
	
	// Set the 'presentation time' and 'duration' of this frame:
	if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
		// This is the first frame, so use the current time:
		gettimeofday(&fPresentationTime, NULL);
	} else {
		// Increment by the play time of the previous data:
		unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
		fPresentationTime.tv_sec += uSeconds/1000000;
		fPresentationTime.tv_usec = uSeconds%1000000;
	}

  	// Remember the play time of this data:
	fDurationInMicroseconds = fLastPlayTime = (unsigned)((fPlayTimePerSample*fFrameSize)/bytesPerSample);
	

	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
			(TaskFunc*)FramedSource::afterGetting, this);
}

