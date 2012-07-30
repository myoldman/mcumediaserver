#include "rtp.h"
#include "h264/h264depacketizer.h"


 RTPDepacketizer* RTPDepacketizer::Create(MediaFrame::Type mediaType,DWORD codec)
 {
	 switch (mediaType)
	 {
		 case MediaFrame::Video:
			 //Depending on the codec
			 switch((VideoCodec::Type)codec)
			 {
				 case VideoCodec::H264:
					 return new H264Depacketizer();
			 }
			 break;
		 case MediaFrame::Audio:
			 break;
	 }
	 return NULL;
 }
 