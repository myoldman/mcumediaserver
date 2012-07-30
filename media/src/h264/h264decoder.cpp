#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "log.h"
#include "h264decoder.h"

static const char base64Char[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char* base64Encode(char const* origSigned, unsigned origLength) 
{  
	unsigned char const* orig = (unsigned char const*)origSigned; 
	// in case any input bytes have the MSB set 
	if (orig == NULL) return NULL;  
	unsigned const numOrig24BitValues = origLength/3;  
	bool havePadding = origLength > numOrig24BitValues*3;  
	bool havePadding2 = origLength == numOrig24BitValues*3 + 2;  
	unsigned const numResultBytes = 4*(numOrig24BitValues + havePadding); 
	char* result = new char[numResultBytes+1]; 
	// allow for trailing '\0'  
	// Map each full group of 3 input bytes into 4 output base-64 characters:  
	unsigned i;  
	for (i = 0; i < numOrig24BitValues; ++i) 
	{    
		result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];    
		result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];    
		result[4*i+2] = base64Char[((orig[3*i+1]<<2) | (orig[3*i+2]>>6))&0x3F];    
		result[4*i+3] = base64Char[orig[3*i+2]&0x3F];  
	}  

	// Now, take padding into account.  (Note: i == numOrig24BitValues)  
	if (havePadding) 
	{    
		result[4*i+0] = base64Char[(orig[3*i]>>2)&0x3F];    
		if (havePadding2) 
		{      
			result[4*i+1] = base64Char[(((orig[3*i]&0x3)<<4) | (orig[3*i+1]>>4))&0x3F];
			result[4*i+2] = base64Char[(orig[3*i+1]<<2)&0x3F];    
		} else {     
			result[4*i+1] = base64Char[((orig[3*i]&0x3)<<4)&0x3F];      
			result[4*i+2] = '=';    
		}    
		result[4*i+3] = '=';  
	}  
	result[numResultBytes] = '\0';  
	return result;
}

//H264Decoder
// 	Decodificador H264
//
//////////////////////////////////////////////////////////////////////////
/***********************
* H264Decoder
*	Consturctor
************************/
H264Decoder::H264Decoder()
{
	// add for rtsp watcher by liuhong start
	memset(sps_base64, 0, sizeof(sps_base64));
	memset(pps_base64, 0, sizeof(pps_base64));
	rtspReady = false;
	idrSaved = false;
	// add for rtsp watcher by liuhong end
	type = VideoCodec::H264;

	//Guardamos los valores por defecto
	codec = NULL;
	buffer = 0;
	bufLen = 0;
	bufSize = 0;
	ctx = NULL;
	picture = NULL;
	
	//Registramos todo
	avcodec_register_all();

	//Encotramos el codec
	codec = avcodec_find_decoder(CODEC_ID_H264);

	//Comprobamos
	if(codec==NULL)
	{
		Error("No decoder found\n");
		return;
	}

	//Alocamos el contxto y el picture
	ctx = avcodec_alloc_context();
	ctx->flags != CODEC_FLAG_EMU_EDGE;
	picture = avcodec_alloc_frame();

	//Alocamos el buffer
	bufSize = 1024*756*3/2;
	buffer = (BYTE *)malloc(bufSize);
	frame = NULL;
	frameSize = 0;
	src = 0;
	
	spsBuffer = NULL;
	spsBufferLen = 0;
	ppsBuffer = NULL;
	ppsBufferLen = 0;
	//Lo abrimos
	avcodec_open(ctx, codec);
}

/***********************
* ~H264Decoder
*	Destructor
************************/
H264Decoder::~H264Decoder()
{
	if (buffer)
		free(buffer);
	if (frame)
		free(frame);
	if (ctx)
	{
		avcodec_close(ctx);
		free(ctx);
	}
	if (picture)
		free(picture);
	
	//Emtpy
	while (!idrPacket.empty())
	{
		//Delete
		delete[] idrPacket.back();
		//remove
		idrPacket.pop_back();
	}

	idrPacketSize.clear();
}
/* 3 zero bytes syncword */
static const uint8_t sync_bytes[] = { 0, 0, 0, 1 };

void H264Decoder::recontructIdrPacket()
{
	//clear idr packet saved
	while (!idrPacket.empty())
	{
		//Delete
		delete[] idrPacket.back();
		//remove
		idrPacket.pop_back();
	}
	idrPacketSize.clear();
								
	BYTE *spsPacket = (BYTE*) malloc(spsBufferLen);
	memcpy(spsPacket, spsBuffer, spsBufferLen);
	idrPacket.push_back(spsPacket);
	idrPacketSize.push_back(spsBufferLen);

	BYTE *ppsPacket = (BYTE*) malloc(ppsBufferLen);
	memcpy(ppsPacket, ppsBuffer, ppsBufferLen);
	idrPacket.push_back(ppsPacket);
	idrPacketSize.push_back(ppsBufferLen);
}

void H264Decoder::copyIdrPacketFromTemp() {
	size_t packetSize = idrPacketTemp.size();
	for (size_t i = 0; i< packetSize ; ++i) 
	{
		BYTE *packet = idrPacketTemp[i];
		int length = idrPacketSizeTemp[i];
		idrPacket.push_back(packet);
		idrPacketSize.push_back(length);
	}
	idrPacketTemp.clear();
	idrPacketSizeTemp.clear();
}

DWORD H264Decoder::h264_append_nals(BYTE *dest, DWORD destLen, DWORD destSize, BYTE *buffer, DWORD bufferLen, BYTE **nals, DWORD nalSize, DWORD *num, int last)
{
	BYTE nal_unit_type;
	unsigned int header_len;
	BYTE nal_ref_idc;
	unsigned int nalu_size;

	DWORD payload_len = bufferLen;
	BYTE *payload = buffer;
	BYTE *outdata = dest+destLen;
	DWORD outsize = 0;

	//No nals
	*num = 0;

	//Check
	if (!bufferLen)
		//Exit
		return 0;


	/* +---------------+
	 * |0|1|2|3|4|5|6|7|
	 * +-+-+-+-+-+-+-+-+
	 * |F|NRI|  Type   |
	 * +---------------+
	 *
	 * F must be 0.
	 */
	nal_ref_idc = (payload[0] & 0x60) >> 5;
	nal_unit_type = payload[0] & 0x1f;
	//printf("[NAL:%x,type:%x]\n", payload[0], nal_unit_type);

	/* at least one byte header with type */
	header_len = 1;

	switch (nal_unit_type) 
	{
		case 0:
		case 30:
		case 31:
			/* undefined */
			return 0;
		case 25:
			/* STAP-B		Single-time aggregation packet		 5.7.1 */
			/* 2 byte extra header for DON */
			/** Not supported */
			return 0;	
		case 24:
		{
			/**
			   Figure 7 presents an example of an RTP packet that contains an STAP-
			   A.  The STAP contains two single-time aggregation units, labeled as 1
			   and 2 in the figure.

			       0                   1                   2                   3
			       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			      |                          RTP Header                           |
			      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			      |STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
			      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			      |                         NALU 1 Data                           |
			      :                                                               :
			      +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			      |               | NALU 2 Size                   | NALU 2 HDR    |
			      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			      |                         NALU 2 Data                           |
			      :                                                               :
			      |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			      |                               :...OPTIONAL RTP padding        |
			      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

			      Figure 7.  An example of an RTP packet including an STAP-A and two
					 single-time aggregation units
			*/

			/* Skip STAP-A NAL HDR */
			payload++;
			payload_len--;
			
			/* STAP-A Single-time aggregation packet 5.7.1 */
			while (payload_len > 2) 
			{
				/* Get NALU size */
				nalu_size = (payload[0] << 8) | payload[1];

				/* strip NALU size */
				payload += 2;
				payload_len -= 2;

				if (nalu_size > payload_len)
					nalu_size = payload_len;

				outsize += nalu_size + sizeof (sync_bytes);

				/* Check size */
				if (outsize + destLen >destSize)
					return Error("Frame to small to add NAL [%d,%d,%d]\n",outsize,destLen,destSize);

				memcpy (outdata, sync_bytes, sizeof (sync_bytes));
				outdata += sizeof (sync_bytes);

				//Set nal
				if (nals && nalSize-1>*num)
					//Add it
					nals[(*num)++] = outdata;

				memcpy (outdata, payload, nalu_size);
				outdata += nalu_size;

				payload += nalu_size;
				payload_len -= nalu_size;
			}

			return outsize;
		}
		case 26:
			/* MTAP16 Multi-time aggregation packet	5.7.2 */
			header_len = 5;
			return 0;
			break;
		case 27:
			/* MTAP24 Multi-time aggregation packet	5.7.2 */
			header_len = 6;
			return 0;
			break;
		case 28:
		case 29:
		{
			/* FU-A	Fragmentation unit	 5.8 */
			/* FU-B	Fragmentation unit	 5.8 */
			BYTE S, E;

			/* +---------------+
			 * |0|1|2|3|4|5|6|7|
			 * +-+-+-+-+-+-+-+-+
			 * |S|E|R| Type	   |
			 * +---------------+
			 *
			 * R is reserved and always 0
			 */
			S = (payload[1] & 0x80) == 0x80;
			E = (payload[1] & 0x40) == 0x40;
			BYTE fu_nal_unit_type = (payload[1] & 0x1f);
			
			if( fu_nal_unit_type == 5 && ppsBufferLen > 0 && spsBufferLen > 0)
			{
				BYTE *savedPacket = (BYTE*) malloc(payload_len);
				memcpy(savedPacket, payload, payload_len);
				idrPacketTemp.push_back(savedPacket);
				idrPacketSizeTemp.push_back(payload_len);
				if(last)
				{
					recontructIdrPacket();
					copyIdrPacketFromTemp();
					Log("FUA Idr packet updated %d\n", idrPacketSize.size());
				}
			}

			if (S) 
			{
				/* NAL unit starts here */
				BYTE nal_header;

				/* reconstruct NAL header */
				nal_header = (payload[0] & 0xe0) | (payload[1] & 0x1f);
				
				

				/* strip type header, keep FU header, we'll reuse it to reconstruct
				 * the NAL header. */
				payload += 1;
				payload_len -= 1;

				nalu_size = payload_len;
				outsize = nalu_size + sizeof (sync_bytes);

				//Check size
				if (outsize + destLen >destSize)
					return Error("Frame too small to add NAL [%d,%d,%d]\n",outsize,destLen,destSize);
				
				memcpy (outdata, sync_bytes, sizeof (sync_bytes));
				outdata += sizeof (sync_bytes);
				
				//Set nal
				if (nals && nalSize-1>*num)
					//Add it
					nals[(*num)++] = outdata;

				memcpy (outdata, payload, nalu_size);
				outdata[0] = nal_header;
				outdata += nalu_size;
				return outsize;

			} else {
				/* strip off FU indicator and FU header bytes */
				payload += 2;
				payload_len -= 2;

				outsize = payload_len;
				//Check size
				if (outsize + destLen >destSize)
					return Error("Frame too small to add NAL [%d,%d,%d]\n",outsize,destLen,destSize);
				memcpy (outdata, payload, outsize);
				outdata += nalu_size;
				return outsize;
			}

			break;
		}
		default:
		{
			// add for rtsp watcher by liuhong start
			// sps
			if(nal_unit_type == 7)
			{
				spsBuffer = (BYTE*) malloc(payload_len);
				memcpy(spsBuffer, payload, payload_len);
				spsBufferLen = payload_len;
 				char* sps_base64 = base64Encode((char*)payload, payload_len);
				strcpy(this->sps_base64, sps_base64);
				fprintf(stderr, "sps is %s\n", this->sps_base64);
			}
			
			// pps
			if(nal_unit_type == 8)
			{
				ppsBuffer = (BYTE*) malloc(payload_len);
				memcpy(ppsBuffer, payload, payload_len);
				ppsBufferLen = payload_len;
 				char* pps_base64 = base64Encode((char*)payload, payload_len);
				strcpy(this->pps_base64, pps_base64);
				fprintf(stderr, "pps is %s\n", this->pps_base64);
			}
			
			if (nal_unit_type == 8 && ppsBufferLen > 0 && spsBufferLen > 0)
			{
				rtspReady = true;
				recontructIdrPacket();
			}

			if(nal_unit_type == 5  && ppsBufferLen > 0 && spsBufferLen > 0) {
				BYTE *savedPacket = (BYTE*) malloc(payload_len);
				memcpy(savedPacket, payload, payload_len);
				idrPacketTemp.push_back(savedPacket);
				idrPacketSizeTemp.push_back(payload_len);
				if(last)
				{
					recontructIdrPacket();
					copyIdrPacketFromTemp();
					Log("Idr packet updated %d\n", idrPacketSize.size());
				}
			}
			// add for rtsp watcher by liuhong end

			/* 1-23	 NAL unit	Single NAL unit packet per H.264	 5.6 */
			/* the entire payload is the output buffer */
			nalu_size = payload_len;
			outsize = nalu_size + sizeof (sync_bytes);
			//Check size
			if (outsize + destLen >destSize)
				return Error("Frame too small to add NAL [%d,%d,%d]\n",outsize,destLen,destSize);
			memcpy (outdata, sync_bytes, sizeof (sync_bytes));
			outdata += sizeof (sync_bytes);

			//Set nal
			if (nals && nalSize-1>*num)
				//Add it
				nals[(*num)++] = outdata;

			memcpy (outdata, payload, nalu_size);
			outdata += nalu_size;

			return outsize;
		}
	}

	return 0;
}

DWORD H264Decoder::h264_append(BYTE *dest, DWORD destLen, DWORD destSize, BYTE *buffer, DWORD bufferLen, int last)
{
	DWORD num = 0;
	h264_append_nals(dest,destLen,destSize,buffer,bufferLen,NULL,0,&num, last);
}

/***********************
* DecodePacket 
*	Decodifica un packete
************************/
int H264Decoder::DecodePacket(BYTE *in,DWORD inLen,int lost,int last)
{
	int ret = 1;
	
	// Check total length
	if (bufLen+inLen+FF_INPUT_BUFFER_PADDING_SIZE>bufSize)
	{
		// Reset buffer
		bufLen = 0;

		// Exit
		return 0;
	}

	//Aumentamos la longitud
	bufLen += h264_append(buffer,bufLen,bufSize-FF_INPUT_BUFFER_PADDING_SIZE,in,inLen, last);

	//Si es el ultimo
	if(last)
	{
		//Borramos el final
		memset(buffer+bufLen,0,FF_INPUT_BUFFER_PADDING_SIZE);
		//Decode
		//ret = Decode(buffer,bufLen);
		//Y resetamos el buffer
		bufLen=0;
		ret = 1;
	} else {
		ret = 1;
	}
	//Return
	return ret;
}

int H264Decoder::Decode(BYTE *buffer,DWORD size)
{
	//Decodificamos
	int got_picture=0;
	//Decodificamos
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = buffer;
	pkt.size = size;
	int readed = avcodec_decode_video2(ctx, picture, &got_picture, &pkt);

	//Si hay picture
	if (got_picture && readed>0)
	{
		if(ctx->width==0 || ctx->height==0)
			return Error("-Wrong dimmensions [%d,%d]\n",ctx->width,ctx->height);

		int w = ctx->width;
		int h = ctx->height;
		int u = w*h;
		int v = w*h*5/4;
		int size = w*h*3/2;

		//Comprobamos el tama�o
		if (size>frameSize)
		{
			Log("-Frame size %dx%d\n",w,h);
			//Liberamos si habia
			if(frame!=NULL)
				free(frame);
			//Y allocamos de nuevo
			frame = (BYTE*) malloc(size);
			frameSize = size;
		}

		//Copaamos  el Cy
		for(int i=0;i<ctx->height;i++)
			memcpy(&frame[i*w],&picture->data[0][i*picture->linesize[0]],w);

		//Y el Cr y Cb
		for(int i=0;i<ctx->height/2;i++)
		{
			memcpy(&frame[i*w/2+u],&picture->data[1][i*picture->linesize[1]],w/2);
			memcpy(&frame[i*w/2+v],&picture->data[2][i*picture->linesize[2]],w/2);
		}
	}
	return 1;
}

