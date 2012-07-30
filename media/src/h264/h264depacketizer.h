/* 
 * File:   h264depacketizer.h
 * Author: Sergio
 *
 * Created on 26 de enero de 2012, 9:46
 */

#ifndef H264DEPACKETIZER_H
#define	H264DEPACKETIZER_H
#include "rtp.h"

class H264Depacketizer : public RTPDepacketizer
{
public:
	H264Depacketizer();
	virtual ~H264Depacketizer();
	virtual MediaFrame* AddPacket(RTPPacket *packet);
	virtual MediaFrame* AddPayload(BYTE* payload,DWORD payload_len);
	virtual void ResetFrame();
	virtual MediaFrame* GetFrame();
private:
	VideoFrame frame;
};

#endif	/* H264DEPACKETIZER_H */

