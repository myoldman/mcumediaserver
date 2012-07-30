/**
* @file RtspWatcher.hh
* @brief Rtsp Watcher for a mcu conference
* @author Hong Liu<myoldman@163.com>
* @version 1.0
* @history
* 1. created on 2012-1-10
*/
#ifndef _RTSP_WATCHER_H_
#define _RTSP_WATCHER_H_
#include <pthread.h>
#include "video.h"
#include "audio.h"
#include "codecs.h"
#include "rtpsession.h"

class H264MediaSubsession;
class WavMediaSubsession;
class MultiConf;

class RtspWatcher
{
public:
	RtspWatcher();
	~RtspWatcher();
	
	/**
	* @brief Set video codec of rtsp stream
	* @param codec video codec
	* @param mode capture mode(CIF VGA PAL...)
	* @param fps frame per second
	* @param bitrate 
	* @param quality codec quality 
	* @param fillLevel codec fill Level
	* @param intraPeriod codec key frame period
	* @return setting result
	*/
	int SetVideoCodec(VideoCodec::Type codec,int mode,int fps,int bitrate,int quality=0, int fillLevel=0,int intraPeriod=0);

	/**
	* @brief Set audio codec of rtsp stream
	* @param codec audio codec
	* @return setting result
	*/
	int SetAudioCodec(AudioCodec::Type codec);

	/**
	* @brief Init rtsp watcher with video input and audio input
	* @param audioInput audio input frame gruber
	* @param videoInput video input frame gruber
	* @return setting result
	*/
	int Init(AudioInput* audioInput,VideoInput *videoInput, const std::string& streamName, MultiConf *conf);
	
	/**
	* @brief Start stream conference
	* @return setting result
	*/
	int StartStreaming();
		
	/**
	* @brief Stop stream conference
	* @return setting result
	*/
	int StopStreaming();

	/**
	* @brief End of rtsp watcher
	* @return setting result
	*/
	int End();

	RTPSession *StartSendingVideo(unsigned clientSessionId,char *sendVideoIp,int sendVideoPort,VideoCodec::RTPMap& rtpMap);

	int StopSendingVideo(unsigned clientSessionId);

	RTPSession *StartSendingAudio(unsigned clientSessionId,char* sendAudioIp,int sendAudioPort,AudioCodec::RTPMap& rtpMap);

	int StopSendingAudio(unsigned clientSessionId);

	RTPSession *StartRecvingVideoRtcp(unsigned clientSessionId);
	RTPSession *StartRecvingAudioRtcp(unsigned clientSessionId);

	H264MediaSubsession* GetH264MediaSubsession() { return videoSubsession;}
	void SetH264MediaSubsession(H264MediaSubsession* videoSubsession) { this->videoSubsession = videoSubsession;}
	WavMediaSubsession* GetWavMediaSubsession() { return audioSubsession;}
	void SetWavMediaSubsession(WavMediaSubsession* audioSubsession) { this->audioSubsession = audioSubsession;}

	VideoInput* GetVideoInput() { return videoInput;}
	AudioInput* GetAudioInput() { return audioInput;}
	int GetVideoGrabWidth() {return videoGrabWidth;}
	int GetVideoGrabHeight() {return videoGrabHeight;}
	int GetVideoFPS() {return videoFPS;}
	int GetVideoBitrate() {return videoBitrate;}
	int GetVideoQuality() {return videoQuality;}
	int GetVideoFillLevel() {return videoFillLevel;}
	int GetVideoIntraPeriod() {return videoIntraPeriod;}
	int GetNumFrameSamples() {return numFrameSamples;}
	int GetSamplingFrequency() {return fSamplingFrequency;}
	int GetBitsPerSample() {return fBitsPerSample;}
	int GetNumChannels() {return fNumChannels;}
	AudioCodec::Type GetAudioCodec() {return audioCodec;}
	VideoCodec::Type GetVideoCodec() {return videoCodec;}
private:
	
	AudioInput*		audioInput;
	VideoInput*		videoInput;

	AudioCodec::Type	audioCodec;
	VideoCodec::Type	videoCodec;
	
	int		videoCaptureMode;	//Modo de captura de video actual
	int 		videoGrabWidth;		//Ancho de la captura
	int 		videoGrabHeight;	//Alto de la captur
	int 		videoFPS;
	int 		videoBitrate;
	int		videoQuality;
	int		videoFillLevel;
	int		videoIntraPeriod;
	int		inited;	
	int		numFrameSamples;
	int 		fSamplingFrequency;
	int 		fNumChannels;
	int 		fBitsPerSample;
	std::string 		m_StreamName;
	H264MediaSubsession* videoSubsession;
	WavMediaSubsession* audioSubsession;
	MultiConf*	m_Conf;
};

#endif

