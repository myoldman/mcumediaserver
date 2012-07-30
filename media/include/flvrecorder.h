#ifndef _FLVRECORDER_H_
#define _FLVRECORDER_H_
#include "flv.h"
#include "rtmpmessage.h"

class FLVRecorder
{
public:
	FLVRecorder();
	~FLVRecorder();
	bool Create(const char *filename);
	bool Record();
	bool Write(RTMPMediaFrame *frame);
	bool Set(RTMPMetaData *meta);
	bool Close();

private:
	int	fd;
	DWORD	offset;
	bool	recording;
	QWORD 	first;
	QWORD	last;
	RTMPMetaData *meta;
};

#endif
