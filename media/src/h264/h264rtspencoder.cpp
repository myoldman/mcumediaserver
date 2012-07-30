#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "log.h"
#include "h264rtspencoder.h"

H264RtspEncoder::H264RtspEncoder(int qualityMin,int qualityMax) : H264Encoder(qualityMin, qualityMax)
{
}

H264RtspEncoder::~H264RtspEncoder()
{
}
//////////////////////////////////////////////////////////////////////////
//Encoder
// 	Codificador H264
//
//////////////////////////////////////////////////////////////////////////

static void X264_log(void *p, int level, const char *fmt, va_list args)
{
	vprintf(fmt, args);
}

/**********************
* OpenCodec
*	Abre el codec
***********************/
int H264RtspEncoder::OpenCodec()
{
	x264_param_t    params;

	Log("-OpenCodec H264 [%dbps,%dfps]\n",bitrate,fps);

	// Check 
	if (opened)
		return Error("Codec already opened\n");

	// Reset default values
	x264_param_default(&params);

	// Use a defulat preset
	x264_param_default_preset(&params,"fast","zerolatency");


	// Set log
	params.pf_log               = X264_log;
	params.i_log_level          = X264_LOG_WARNING;

	// Set encoding context size
	params.i_width 	= width;
	params.i_height	= height;

	// Set parameters
	params.i_keyint_max         = intraPeriod;
	params.i_keyint_min         = 1;
	params.b_cabac = 		1;
	params.i_frame_reference    = 0;
	params.rc.i_bitrate         = bitrate / 1024;
	params.rc.b_stat_write      = 0;
	params.i_slice_max_size     = RTPPAYLOADSIZE;
	params.b_sliced_threads	    = 0;
	params.rc.i_lookahead       = 0;
	params.i_bframe             = 0;
	//params.b_annexb		    = 0; 
	params.b_repeat_headers     = 1;
	params.i_threads	    = 0;
	params.b_vfr_input    = 0; 

	params.rc.i_qp_min          = qMin;
	params.rc.i_qp_max          = qMax;
	params.rc.i_qp_step         = qMax-qMin;

	params.i_fps_num 		= fps;
	params.i_fps_den 		= 2;

	// Set profile level constrains
	x264_param_apply_profile(&params,"baseline");

	// Open encoder
	enc = x264_encoder_open(&params);

	//Check it is correct
	if (!enc)
		return Error("Could not open h264 codec\n");

	// Clean pictures
	memset(&pic,0,sizeof(x264_picture_t));
	memset(&pic_out,0,sizeof(x264_picture_t));

	//Set picture type
	pic.i_type = X264_TYPE_AUTO;
	
	// We are opened
	opened=true;

	// Exit
	return 1;
}
