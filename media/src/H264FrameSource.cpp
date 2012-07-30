/**
* @file H264FrameSource.cpp
* @brief H264 Frame source for live555 rtsp subsession
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/
#include <H264FrameSource.hh>
#include "log.h"
#include "h264/h264encoder.h"

H264FrameSource::H264FrameSource(UsageEnvironment &env, RtspWatcher *rtspWatcher) : FramedSource(env)
{
	m_videoInput = rtspWatcher->GetVideoInput();
	m_started = 0;
        mp_token = NULL;
	videoFPS = rtspWatcher->GetVideoFPS();
	sendFPU = true;
	//No wait for first
	frameTime = 0;

	//The time of the first one
	gettimeofday(&first,NULL);

	//The time of the previos one
	gettimeofday(&prev,NULL);


	videoEncoder = VideoCodecFactory::CreateEncoder(rtspWatcher->GetVideoCodec());
	videoEncoder->SetFrameRate((float)videoFPS,rtspWatcher->GetVideoBitrate(), rtspWatcher->GetVideoIntraPeriod());
	videoEncoder->SetSize(rtspWatcher->GetVideoGrabWidth(),rtspWatcher->GetVideoGrabHeight());
	fLastPlayTime = 0;
	Log("-----Rstp H264 frame source created\n");
}


H264FrameSource::~H264FrameSource()
{
	Log("-----Rstp H264 frame source destroy\n");
	if (m_started) 
	{
            envir().taskScheduler().unscheduleDelayedTask(mp_token);
        }
	if(videoEncoder) 
	{
		delete  videoEncoder;
	}
}

void H264FrameSource::End()
{
	if(m_videoInput)
	{
		m_videoInput = NULL;
	}
}

void H264FrameSource::doGetNextFrame()
{  
        // 根据 fps, 计算等待时间  
        double delay = 1000.0 / videoFPS ;
        int to_delay = delay * 1000;    // us  
  
        if(!m_videoInput)
		return;

	BYTE *pic = m_videoInput->GrabFrame();

	//Check picture
	if (!pic) {
		fFrameSize = 0;
		m_started = 0; 
		return;
	}

	//Check if we need to send intra
	if (sendFPU)
	{
		videoEncoder->FastPictureUpdate();
	}

	//if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
      		// This is the first frame, so use the current time:
      		
	//} else {
		// Increment by the play time of the previous data:
	//	unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
	//	fPresentationTime.tv_sec += uSeconds/1000000;
	//	fPresentationTime.tv_usec = uSeconds%1000000;
	//}
	
	// Remember the play time of this data:
	//fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
	//fDurationInMicroseconds = fLastPlayTime;
	//fDurationInMicroseconds = 1000.0 / videoFPS;

	VideoFrame *videoFrame = videoEncoder->EncodeFrame(pic,m_videoInput->GetBufferSize());
	
	//If was failed
	if (!videoFrame){
		//Next
		fFrameSize = 0;
		m_started = 0;
		Log("-----Error encoding video\n");
        	double delay = 1000.0 / videoFPS;
        	int to_delay = delay * 1000;    // us  
        	nextTask() = envir().taskScheduler().scheduleDelayedTask(to_delay,
                (TaskFunc*)FramedSource::afterGetting, this); 
		return;
	}
	
	if(sendFPU)
		sendFPU = false;

	//Set frame timestamp
	videoFrame->SetTimestamp(getDifTime(&first)/1000);

	//Set sending time of previous frame
	//getUpdDifTime(&prev);

	//gettimeofday(&fPresentationTime, 0);

	fFrameSize = videoFrame->GetLength();

	memmove(fTo, videoFrame->GetData(), fFrameSize);

	if (fFrameSize > fMaxSize) {
		fNumTruncatedBytes = fFrameSize - fMaxSize;
		fFrameSize = fMaxSize;
	}
	else {
		fNumTruncatedBytes = 0;
	}
	
	gettimeofday(&fPresentationTime, NULL);

	//to_delay = ((1000 / videoFPS) * fFrameSize / RTPPAYLOADSIZE) * 1000;    // us  

        nextTask() = envir().taskScheduler().scheduleDelayedTask(to_delay,
				(TaskFunc*)FramedSource::afterGetting, this);
	
}

unsigned H264FrameSource::maxFrameSize() const
{
	return 100 * 1024;
}
