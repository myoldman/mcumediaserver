/**
* @file H264FrameSource.hh
* @brief H264 Frame source for live555 rtsp subsession
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/

#ifndef _H264_FRAME_SOURCE_HH
#define _H264_FRAME_SOURCE_HH
#include <liveMedia.hh>
#include <FramedSource.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "video.h"
#include "RtspWatcher.hh"

class H264FrameSource : public FramedSource
{
	public: 
		H264FrameSource (UsageEnvironment &env, RtspWatcher *rtspWatcher);
		virtual ~H264FrameSource ();
		void End();
	protected:  
		virtual void doGetNextFrame ();
		virtual unsigned maxFrameSize() const;

	private:
		int m_started;
    		void *mp_token;
		int 	videoFPS;
		bool	sendFPU;
		timeval first;
		timeval prev;	
		unsigned fLastPlayTime;
		//No wait for first
		DWORD frameTime;
		VideoInput *m_videoInput;
		VideoEncoder *videoEncoder;
		
};
#endif

