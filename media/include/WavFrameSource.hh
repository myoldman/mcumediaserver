/**
* @file WavFrameSource.hh
* @brief Wav Frame source for live555 rtsp subsession
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#ifndef _WAV_FRAME_SOURCE_HH
#define _WAV_FRAME_SOURCE_HH
#include <liveMedia.hh>
#include <FramedSource.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "audio.h"
#include "codecs.h"
#include "RtspWatcher.hh"

class WavFrameSource : public FramedSource
{
	public: 
		WavFrameSource (UsageEnvironment &env, RtspWatcher *rtspWatcher);
		virtual ~WavFrameSource ();
		void End();
	protected:  
		virtual void doGetNextFrame ();

	private:
		AudioInput *m_audioInput;
		AudioCodec *audioEncoder;
		int 		fSamplingFrequency;
		int 		fNumChannels;
		int 		fBitsPerSample;
		double 		fPlayTimePerSample; // useconds
		unsigned 	fLastPlayTime; // useconds
		int		fNumFrameSamples;
		
};
#endif
