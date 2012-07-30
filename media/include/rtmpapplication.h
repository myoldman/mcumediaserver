#ifndef _RTMP_APPLICATION_H_
#define _RTMP_APPLICATION_H_
#include "config.h"
#include "rtmpstream.h"
#include <string>

class RTMPApplication
{
public:
	virtual RTMPStream* CreateStream(DWORD streamId, std::wstring& appName,DWORD audioCaps,DWORD videoCaps) = 0;
};

#endif
