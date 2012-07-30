#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <pthread.h>

#define LIMIT(x) ((x)>0xffffff?0xff: ((x)<=0xffff?0:((x)>>16)))

/*************************************
* blocksignals
*       Bloquea todas las sygnals para esa thread
*************************************/
int blocksignals()
{
	sigset_t mask;

        //Close it
        sigemptyset(&mask);
	//Solo cogemos el control-c
	sigaddset(&mask,SIGINT);
	sigaddset(&mask,SIGUSR1);

	//y bloqueamos
	return pthread_sigmask(SIG_BLOCK,&mask,0);
}

/*************************************
* createPriorityThread
*       Crea un nuevo hilo y le asigna la prioridad dada
*************************************/
int createPriorityThread(pthread_t *thread, void *(*function)(void *), void *arg, int priority) 
{
	struct sched_param parametros;
	parametros.sched_priority = priority;
		
	//Creamos el thread	
	if (pthread_create(thread,NULL,function,arg) != 0)
		return 0;
        
	return 1;
	/*
	 * //Aumentamos la prioridad
	if (pthread_setschedparam(*thread,SCHED_RR,&parametros) != 0)
		return 0;

	return 1;
	*/
}


/************************************
* msleep	
*	Duerme la cantidad de micro segundos usando un select
*************************************/
int msleep(long msec)
{
	struct timeval tv;

	tv.tv_sec=(int)((float)msec/1000000);
	tv.tv_usec=msec-tv.tv_sec*1000000;
	return select(0,0,0,0,&tv);
}

/*********************************
* getDifTime
*	Obtiene cuantos microsegundos ha pasado desde "antes"
*********************************/
QWORD getDifTime(struct timeval *before)
{
	//Obtenemos ahora
	struct timeval now;
	gettimeofday(&now,0);

	//Ahora calculamos la diferencia
	return (((QWORD)now.tv_sec)*1000000+now.tv_usec) - (((QWORD)before->tv_sec)*1000000+before->tv_usec);
}

/*********************************
* getUpdDifTime
*	Calcula la diferencia entre "antes" y "ahora" y actualiza "antes"
*********************************/
QWORD getUpdDifTime(struct timeval *before)
{
	//Ahora calculamos la diferencia
	QWORD dif = getDifTime(before);

	//Actualizamos before
	gettimeofday(before,0);

	return dif;
}

/*********************************
* setZeroTime
*	Set time val to 0
*********************************/
void setZeroTime(struct timeval *val)
{
        //Fill with zeros
	val->tv_sec = 0;
	val->tv_usec = 0;
}

/*********************************
* isZeroTime
*	Check if time val is 0
*********************************/
DWORD isZeroTime(struct timeval *val)
{
        //Check if it is zero
        return !(val->tv_sec & val->tv_usec);
}

/*****************************************
 * calcAbsTimout
 *      Create timespec value to be used in timeout calls
 **************************************/
void calcAbsTimeout(struct timespec *ts,struct timeval *tp,DWORD timeout)
{
	ts->tv_sec  = tp->tv_sec + timeout/1000;
	ts->tv_nsec = tp->tv_usec * 1000 + (timeout%1000)*1000000;

	//Check overflowns
	if (ts->tv_nsec>=1000000000)
	{
		//Decrease
		ts->tv_nsec -= 1000000000;
		//Inc seconds
		ts->tv_sec++;
	}
}

/*****************************************
 * calcTimout
 *      Create timespec value to be used in timeout calls
 **************************************/
void calcTimout(struct timespec *ts,DWORD timeout)
{
	struct timeval tp;

        //Calculate now
        gettimeofday(&tp, NULL);
        //Increase timeout
        calcAbsTimeout(ts,&tp,timeout);

}

/*********************************
* yuv420p_to_yuv422
*	Convierte una imagen de yuv420p a yuv422
*********************************/
void yuv420p_to_yuv422(int width, int height, BYTE *pIn0, BYTE *pOut0)
{
        int numpix = width * height;
        int i, j;
        BYTE *pY = pIn0;
        BYTE *pU = pY + numpix;
        BYTE *pV = pU + numpix / 4;
        BYTE *buffer = (BYTE *)malloc(numpix*2);;

	BYTE *pOut=buffer;

	//Copiamos las Ys
        for (i = 0; i < numpix; i++) {
                *pOut = *(pY + i);
                pOut += 2;
        }

	//Ahora las Us y Vs
        pOut = buffer + 1;
        for (j = 0; j <= height - 2 ; j += 2) {
                for (i = 0; i <= width - 2; i += 2) {
                        int u = *pU++;
                        int v = *pV++;

                        *pOut = u;
                        *(pOut+2) = v;
                        *(pOut+width*2) = u;
                        *(pOut+width*2+2) = v;
                        pOut += 4;
                }
                pOut += (width * 2);
        }

	//Copiamos
	memcpy(pOut0,buffer,numpix*2);
	printf(".");

	//Liberamos
	free(buffer);
}



/***************************
* reduce_yuv420p_to_rgb565
*	Convierte a un buffer de yuv420p a rgb565 y lo reduce a la mitad si reduce=1
*****************************/
void reduce_yuv420p_to_rgb565(BYTE * image, BYTE *fb,int left,int top,int w, int h,int lineLength,int reduce) 
{
	const int numpix = w*h;
	int i, j, yTL, yTR, yBL, yBR, u, v;
	BYTE *pY = image;
	BYTE *pU = pY + numpix;
	BYTE *pV = pU + numpix / 4;
	BYTE *pOut = fb+left*2+top*lineLength;
	BYTE *line1 = (BYTE *)malloc(w*2);
	BYTE *line2 = (BYTE *)malloc(w*2);
	BYTE *rgb1;
	BYTE *rgb2;
	const int rvScale = 91881;
	const int guScale = -22553;
	const int gvScale = -46801;
	const int buScale = 116129;
	const int yScale  = 65536;
	int r, g, b;

	
	for (j = 0; j <= h - 2; j += 2)
	{
		//Inicilizamos las dos lineas
		rgb1=line1;
		rgb2=line2;

		for (i = 0; i <= w - 2; i += 2)
		{
			yTL = *pY;
			yTR = *(pY + 1);
			yBL = *(pY + w);
			yBR = *(pY + w + 1);
			u = (*pU++) - 128;
			v = (*pV++) - 128;

			g = guScale * u + gvScale * v;
			r = buScale * u;
			b = rvScale * v;
			
			yTL *= yScale; yTR *= yScale;
			yBL *= yScale; yBR *= yScale;


			/* Write out top two pixels */
			rgb1[0] = ((LIMIT(r+yTL) >> 3) & 0x1F)
					| ((LIMIT(g+yTL) << 3) & 0xE0);
			rgb1[1] = ((LIMIT(g+yTL) >> 5) & 0x07)
				| (LIMIT(b+yTL) & 0xF8);

			if(!reduce)
			{
				rgb1[2] = ((LIMIT(r+yTR) >> 3) & 0x1F)
					| ((LIMIT(g+yTR) << 3) & 0xE0);
				rgb1[3] = ((LIMIT(g+yTR) >> 5) & 0x07)
					| (LIMIT(b+yTR) & 0xF8);

				/* Skip down to next line to write out bottom two pixels */
				rgb2[0] = ((LIMIT(r+yBL) >> 3) & 0x1F)
					| ((LIMIT(g+yBL) << 3) & 0xE0);
				rgb2[1] = ((LIMIT(g+yBL) >> 5) & 0x07)
					| (LIMIT(b+yBL) & 0xF8);
				rgb2[2] = ((LIMIT(r+yBR) >> 3) & 0x1F)
					| ((LIMIT(g+yBR) << 3) & 0xE0);
				rgb2[3] = ((LIMIT(g+yBR) >> 5) & 0x07)
					| (LIMIT(b+yBR) & 0xF8);
				rgb1+=4;
				rgb2+=4;
			} else {
				rgb1+=2;
			}

			pY += 2;
		}
		pY += w;

		//Copiamos la primera linea
		memcpy(pOut,line1,rgb1-line1);
		pOut += lineLength;

		//Y la segunda si hace falta
		if (!reduce)
		{
			memcpy(pOut,line2,rgb2-line2);
			pOut += lineLength;

		}
	}

	//Liberamos las lineas
	free(line1);
	free(line2);
}

/*****************
* putPixel
*	Convierte los valores r,g,b,y a rgb con los bpp correspondientes
********************/
inline int putPixel (BYTE *rgb, int r, int g, int b, int y,int bpp)
{
	switch (bpp)
	{
		case 16:
			rgb[0] = ((LIMIT(r+y) >> 3) & 0x1F)
					| ((LIMIT(g+y) << 3) & 0xE0);
			rgb[1] = ((LIMIT(g+y) >> 5) & 0x07)
				| (LIMIT(b+y) & 0xF8);
			break;
		case 24:
			rgb[0] = LIMIT(r+y);
			rgb[1] = LIMIT(g+y);
			rgb[2] = LIMIT(b+y);
			break;
		case 32:
			rgb[0] = LIMIT(r+y);
			rgb[1] = LIMIT(g+y);
			rgb[2] = LIMIT(b+y);
			rgb[3] = 255;
			break;
	}
	return 0;
 }

/*************************
* zoom_yuv420p_to_rgb
*	Convierte un buffer de yuv420p a rgb24 o rgb32
***************************/
void zoom_yuv420p_to_rgb(BYTE * image,BYTE *fb,int sizex,int sizey,int left,int top,int w, int h,int lineLength,int bitspp)
{
	const int numpix = sizex*sizey;
	int i, j, k, yTL, yTR, yBL, yBR, u, v;
	BYTE *pY = image;
	BYTE *pU = pY + numpix;
	BYTE *pV = pU + numpix / 4;
	BYTE *pOut ;
	BYTE *line1;
	BYTE *line2;
	BYTE *zline;
	BYTE *rgb1;
	BYTE *rgb2;
	const int rvScale = 91881;
	const int guScale = -22553;
	const int gvScale = -46801;
	const int buScale = 116129;
	const int yScale  = 65536;
	int r, g, b;
	float zoom=0;
	int iniX,iniY;
	int zj,zi;
	int bpp=0;

	//Si alguno es cero
	if ((w==0) || (h==0) || (sizex==0) || (sizey==0))
		return;

	//El numero de pixels
	switch (bitspp)
	{
		case 16:
			bpp=2;
			break;
		case 24:
			bpp=3;
			break;
		case 32:
			bpp=4;
			break;
	}

	//Los Buffers
	line1 = (BYTE *)malloc(sizex*bpp);
	line2 = (BYTE *)malloc(sizex*bpp);
	zline = (BYTE *)malloc(w*bpp);

	//Calculamos los dos zooms
	float zoomx = (float)w/sizex;
	float zoomy = (float)h/sizey;

	//Nos quedamos siempre con el menor
	if (zoomx>zoomy)
	{
		//Nos quedamos con el zoom vertical
		zoom = zoomy;

		//Los inicios
		iniX = (w - (int)(sizex*zoom))/2;
		iniY = 0;

	} else {
		//Nos quedamos con el zoom horizontal
		zoom = zoomy;

		//Los inicios
		iniX = 0;
		iniY = (h - (int)(sizey*zoom))/2;

		//Rellanamos lo de arriba y abajo de blanco
/*		for (i=0;i<iniY;i++)
		{
			//La linea de arriba
			memset(fb+left*bpp+(top+i)*lineLength,0,w*bpp);
			//La de abajo
			memset(fb+left*bpp+(top+h-i-1)*lineLength,0,w*bpp);
		}*/
	}
	
	//Calculamos el inicio del framebuffer
	pOut = fb+(left+iniX)*bpp+(top+iniY)*lineLength;

	//Y empezampos por la primera linea
	zj=0;	
	
	for (j = 0; j < sizey/2 ; j ++)
	{
		//Inicilizamos las dos lineas
		rgb1=line1;
		rgb2=line2;

		for (i = 0; i < sizex/2 ; i ++)
		{
			yTL = *pY;
			yTR = *(pY + 1);
			yBL = *(pY + sizex);
			yBR = *(pY + sizex + 1);
			u = (*pU++) - 128;
			v = (*pV++) - 128;

			g = guScale * u + gvScale * v;
			r = buScale * u;
			b = rvScale * v;
			
			yTL *= yScale; yTR *= yScale;
			yBL *= yScale; yBR *= yScale;

			/* Write out top two pixels */
			putPixel(rgb1,r,g,b,yTL,bitspp);
			rgb1+=bpp;
			putPixel(rgb1,r,g,b,yTR,bitspp);
			rgb1+=bpp;

			/* Skip down to next line to write out bottom two pixels */
			putPixel(rgb2,r,g,b,yBL,bitspp);
			rgb2+=bpp;
			putPixel(rgb2,r,g,b,yBR,bitspp);
			rgb2+=bpp;

			pY += 2;
		}

		pY +=sizex;

		//Convertimos la primera linea
		memset(zline,0,w*bpp);

		//Hacemos el zoom en cada punto
		for (i=0,zi=0;i<sizex;i++)
			while (((i+1)*zoom>zi) && (zi<w))
			{
				//Copiamos el pixel
				for (k=0;k<bpp;k++)
					zline[zi*bpp+k]=line1[i*bpp+k];

				//Pasamos al siguiente punto
				zi++;
			}
		
		//Hacemos el zoom para la primera linea
		while ((((j*2)+1)*zoom>zj) && (zj<h))
		{
			//Y los compiamos
			memcpy(pOut,zline,w*bpp);
	
			//Inc pout
			pOut += lineLength;

			//Pasamos sigueinte linea
			zj++;
		}

		//Convertimos la segunda linea
		memset(zline,0,w*bpp);

		//Hacemos el zoom en cada punto
		for (i=0,zi=0;i<sizex ;i++)
			while (((i+1)*zoom>zi) && (zi<w))
			{
				//Copiamos el pixel
				for (k=0;k<bpp;k++)
					zline[zi*bpp+k]=line1[i*bpp+k];

				//Pasamos al siguiente punto
				zi++;
			}
		
		//Hacemos el zoom para la segunda linea
		while ((((j*2)+2)*zoom>zj) && (zj<h))
		{
			//Y los compiamos
			memcpy(pOut,zline,w*bpp);
			
			//Inc pout
			pOut += lineLength;

			//Pasamos sigueinte linea
			zj++;
		}
	}

	//Liberamos las lineas
	free(line1);
	free(line2);
	free(zline);
}


/****************************
* UnborderYUV
*	Quita el borde a una imagen YUV
*****************************/
void UnborderYUV (BYTE *pic, int width, int height, int borderWidth, int borderHeight)
{
        int numpix = width * height;
	int i;
        BYTE *pY = pic;
        BYTE *pU = pY + numpix;
        BYTE *pV = pU + numpix / 4;
	BYTE* orig;
	BYTE* dest;

	//Copiamos la Y
	orig = pY+(borderHeight/2)*width+borderWidth/2;
	dest = pY;

	//Para cada linea de Y
	for (i=0;i<(height-borderHeight);i++)
	{
		//Copiamos al origen
		memcpy(dest,orig,(width-borderWidth));
		
		//Movemos los punteros
		orig += width;
		dest += (width-borderWidth);
	}

	//Ahora la U
	orig = pU+(borderHeight/8)*width+borderWidth/4;

	//Para cada linea de Y
	for (i=0;i<(height-borderHeight)/2;i++)
	{
		//Copiamos al origen
		memcpy(dest,orig,(width-borderWidth)/2);
		
		//Movemos los punteros
		orig += width/2;
		dest += (width-borderWidth)/2;
	}

	//Ahora la V
	orig = pV+(borderHeight/8)*width+borderWidth/4;

	//Para cada linea de Y
	for (i=0;i<(height-borderHeight)/2;i++)
	{
		//Copiamos al origen
		memcpy(dest,orig,(width-borderWidth)/2);
		
		//Movemos los punteros
		orig += width/2;
		dest += (width-borderWidth)/2;
	}
}


/*************************
* yuv420p_to_bgr32
*	Convierte un buffer de yuv420p a bgr32
***************************/
void yuv420p_to_bgr32(BYTE * image,BYTE *fb,int sizex,int sizey)
{
	const int numpix = sizex*sizey;
	BYTE *rgb1 = fb;
	BYTE *rgb2;
	BYTE *pY = image;
	BYTE *pU = pY + numpix;
	BYTE *pV = pU + numpix / 4;
	const int rvScale = 91881;
	const int guScale = -22553;
	const int gvScale = -46801;
	const int buScale = 116129;
	const int yScale  = 65536;
	int r, g, b;
	int i, j, k, yTL, yTR, yBL, yBR, u, v;

	for (j = 0; j < sizey/2 ; j ++)
	{
		rgb2 = rgb1 + sizex*4;
		
		for (i = 0; i < sizex/2 ; i ++)
		{
			yTL = *pY;
			yTR = *(pY + 1);
			yBL = *(pY + sizex);
			yBR = *(pY + sizex + 1);
			u = (*pU++) - 128;
			v = (*pV++) - 128;

			g = guScale * u + gvScale * v;
			r = buScale * u;
			b = rvScale * v;
			
			yTL *= yScale; yTR *= yScale;
			yBL *= yScale; yBR *= yScale;

			/* Write out top two pixels */
			putPixel(rgb1,b,g,r,yTL,32);
			rgb1+=4;
			putPixel(rgb1,b,g,r,yTR,32);
			rgb1+=4;

			/* Skip down to next line to write out bottom two pixels */
			putPixel(rgb2,b,g,r,yBL,32);
			rgb2+=4;
			putPixel(rgb2,b,g,r,yBR,32);
			rgb2+=4;

			pY += 2;
		}

		rgb1+=sizex*4;
		pY+=sizex;
	}
}

/***************************
* clip_yuv420p_to_rgb565
*	Convierte a un buffer de yuv420p a rgb565 y lo reduce a la mitad si reduce=1
*****************************/ 
void clip_yuv420p_to_rgb565(BYTE* src, BYTE* dst,int srcX,int srcY,int srcW,int srcH,int srcSizeX,int srcSizeY,int left,int top,int lineLength,int reduce)
{
	const int numpix = srcSizeX*srcSizeY;
	int i, j, yTL, yTR, yBL, yBR, u, v;
	BYTE *pY = src;
	BYTE *pU = pY + numpix;
	BYTE *pV = pU + numpix / 4;
	BYTE *pOut = dst+left*2+top*lineLength;
	BYTE *line1 = (BYTE *)malloc(srcW*2);
	BYTE *line2 = (BYTE *)malloc(srcW*2);
	BYTE *rgb1;
	BYTE *rgb2;
	const int rvScale = 91881;
	const int guScale = -22553;
	const int gvScale = -46801;
	const int buScale = 116129;
	const int yScale  = 65536;
	int r, g, b;

	//Desplazamos al inicio
	pY = pY + srcY*srcSizeX;
	pU = pU + srcY*srcSizeX/4;
	pV = pV + srcY*srcSizeX/4;

	for (j = 0; j <= srcH - 2; j += 2)
	{
		//Inicilizamos las dos lineas
		rgb1=line1;
		rgb2=line2;

		//Nos saltamos el inicio
		pY = pY + srcX;
		pU = pU + srcX/2;
		pV = pV + srcX/2;

		for (i = 0; i <= srcW - 2; i += 2)
		{
			yTL = *pY;
			yTR = *(pY + 1);
			yBL = *(pY + srcSizeX);
			yBR = *(pY + srcSizeX + 1);
			u = (*pU++) - 128;
			v = (*pV++) - 128;

			g = guScale * u + gvScale * v;
			r = buScale * u;
			b = rvScale * v;
			
			yTL *= yScale; yTR *= yScale;
			yBL *= yScale; yBR *= yScale;


			/* Write out top two pixels */
			rgb1[0] = ((LIMIT(r+yTL) >> 3) & 0x1F)
					| ((LIMIT(g+yTL) << 3) & 0xE0);
			rgb1[1] = ((LIMIT(g+yTL) >> 5) & 0x07)
				| (LIMIT(b+yTL) & 0xF8);

			if(!reduce)
			{
				rgb1[2] = ((LIMIT(r+yTR) >> 3) & 0x1F)
					| ((LIMIT(g+yTR) << 3) & 0xE0);
				rgb1[3] = ((LIMIT(g+yTR) >> 5) & 0x07)
					| (LIMIT(b+yTR) & 0xF8);

				/* Skip down to next line to write out bottom two pixels */
				rgb2[0] = ((LIMIT(r+yBL) >> 3) & 0x1F)
					| ((LIMIT(g+yBL) << 3) & 0xE0);
				rgb2[1] = ((LIMIT(g+yBL) >> 5) & 0x07)
					| (LIMIT(b+yBL) & 0xF8);
				rgb2[2] = ((LIMIT(r+yBR) >> 3) & 0x1F)
					| ((LIMIT(g+yBR) << 3) & 0xE0);
				rgb2[3] = ((LIMIT(g+yBR) >> 5) & 0x07)
					| (LIMIT(b+yBR) & 0xF8);
				rgb1+=4;
				rgb2+=4;
			} else {
				rgb1+=2;
			}

			pY += 2;
		}
		pY += srcSizeX;

		//Nos saltamos el final
		pY = pY + srcSizeX - (srcX+srcW);
		pU = pU + (srcSizeX - (srcX+srcW))/2;
		pV = pV + (srcSizeX - (srcX+srcW))/2;

		//Copiamos la primera linea
		memcpy(pOut,line1,rgb1-line1);
		pOut += lineLength;

		//Y la segunda si hace falta
		if (!reduce)
		{
			memcpy(pOut,line2,rgb2-line2);
			pOut += lineLength;

		}
	}

	//Liberamos las lineas
	free(line1);
	free(line2);
}

inline void ZoomLine5to6(BYTE *y,BYTE *pY,int width,int sizeX)
{
	register j;

	//Copiamos el primer
	for (j = 0; j<(width/5); j++)
	{
		y[0] = pY[0];
		y[1] = (1*pY[0] + 5*pY[1])/6;
		y[2] = (2*pY[1] + 4*pY[2])/6;
		y[3] = (3*pY[2] + 3*pY[3])/6;
		y[4] = (4*pY[3] + 2*pY[4])/6;
		y[5] = pY[4];
		y+=6;
		pY+=5;
	}
	for (j=j*6; j<sizeX; j++)
		*(y++) = *(pY++); 
}
inline void ZoomPixels5x6(BYTE *y,BYTE *pY,WORD sizeX,int num)
{
	BYTE * line0 = pY;
	BYTE * line1 = pY + sizeX;
	BYTE * line2 = pY + sizeX*2;
	BYTE * line3 = pY + sizeX*3;
	BYTE * line4 = pY + sizeX*4;

	if (num==1)
	{
		//Line 0
		y[0]	= line0[0];
		
		//Line 1
		y[sizeX]	= ((WORD)(   line0[0]              +  5*line1[0]))/6;

		//Line 2
		y[sizeX*2]	= ((WORD)( 2*line1[0]               +  4*line2[0]))/6;

		//Line 3
		y[sizeX*3]	= ((WORD)( 4*line2[0]               +  2*line3[0]))/6;

		//Line 4
		y[sizeX*4]	= ((WORD)( 5*line3[0]               +   line4[0]))/6;

		//Line 5
		y[sizeX*5]	= line4[0];
	}else if (num==2){
		//Line 0
		y[0]	= line0[0];
		y[1]	= ((WORD)(  line0[0] + 5*line0[1]))/6;
		
		//Line 1
		y[sizeX]	= ((WORD)(   line0[0]              +  5*line1[0]))/6;
		y[1+sizeX]	= ((WORD)(   line0[0] + 5*line0[1] +  5*line1[0] + 25*line1[1]))/36;

		//Line 2
		y[sizeX*2]	= ((WORD)( 2*line1[0]               +  4*line2[0]))/6;
		y[1+sizeX*2]	= ((WORD)( 2*line1[0] + 10*line1[1] +  4*line2[0] + 20*line2[1]))/36;

		//Line 3
		y[sizeX*3]	= ((WORD)( 4*line2[0]               +  2*line3[0]))/6;
		y[1+sizeX*3]	= ((WORD)( 4*line2[0] + 20*line2[1] +  2*line3[0] + 10*line3[1]))/36;

		//Line 4
		y[sizeX*4]	= ((WORD)( 5*line3[0]               +   line4[0]))/6;
		y[1+sizeX*4]	= ((WORD)( 5*line3[0] + 25*line3[1] +   line4[0] + 5*line4[1]))/36;

		//Line 5
		y[sizeX*5]	= line4[0];
		y[1+sizeX*5]	= ((WORD)(  line4[0] + 5*line4[1]))/6;
	}else if (num==3){
		//Line 0
		y[0]	= line0[0];
		y[1]	= ((WORD)(  line0[0] + 5*line0[1]))/6;
		y[2]	= ((WORD)(  line0[1] + 2*line0[2]))/3;
		
		//Line 1
		y[sizeX]	= ((WORD)(   line0[0]              +  5*line1[0]))/6;
		y[1+sizeX]	= ((WORD)(   line0[0] + 5*line0[1] +  5*line1[0] + 25*line1[1]))/36;
		y[2+sizeX]	= ((WORD)(   line0[1] + 2*line0[2] +  5*line1[1] + 10*line1[2]))/18;

		//Line 2
		y[sizeX*2]	= ((WORD)( 2*line1[0]               +  4*line2[0]))/6;
		y[1+sizeX*2]	= ((WORD)( 2*line1[0] + 10*line1[1] +  4*line2[0] + 20*line2[1]))/36;
		y[2+sizeX*2]	= ((WORD)( 2*line1[1] +  4*line1[2] +  4*line2[1] +  8*line2[2]))/18;

		//Line 3
		y[sizeX*3]	= ((WORD)( 4*line2[0]               +  2*line3[0]))/6;
		y[1+sizeX*3]	= ((WORD)( 4*line2[0] + 20*line2[1] +  2*line3[0] + 10*line3[1]))/36;
		y[2+sizeX*3]	= ((WORD)( 4*line2[1] +  8*line2[2] +  2*line3[1] +  4*line3[2]))/18;

		//Line 4
		y[sizeX*4]	= ((WORD)( 5*line3[0]               +   line4[0]))/6;
		y[1+sizeX*4]	= ((WORD)( 5*line3[0] + 25*line3[1] +   line4[0] + 5*line4[1]))/36;
		y[2+sizeX*4]	= ((WORD)( 5*line3[1] + 10*line3[2] +   line4[1] + 2*line4[2]))/18;

		//Line 5
		y[sizeX*5]	= line4[0];
		y[1+sizeX*5]	= ((WORD)(  line4[0] + 5*line4[1]))/6;
		y[2+sizeX*5]	= ((WORD)(  line4[1] + 2*line4[2]))/3;
	}else if (num==4){
		//Line 0
		y[0]	= line0[0];
		y[1]	= ((WORD)(  line0[0] + 5*line0[1]))/6;
		y[2]	= ((WORD)(  line0[1] + 2*line0[2]))/3;
		y[3]	= ((WORD)(2*line0[2] +   line0[3]))/3;
		
		//Line 1
		y[sizeX]	= ((WORD)(   line0[0]              +  5*line1[0]))/6;
		y[1+sizeX]	= ((WORD)(   line0[0] + 5*line0[1] +  5*line1[0] + 25*line1[1]))/36;
		y[2+sizeX]	= ((WORD)(   line0[1] + 2*line0[2] +  5*line1[1] + 10*line1[2]))/18;
		y[3+sizeX]	= ((WORD)( 2*line0[2] +   line0[3] + 10*line1[2] +  5*line1[3]))/18;

		//Line 2
		y[sizeX*2]	= ((WORD)( 2*line1[0]               +  4*line2[0]))/6;
		y[1+sizeX*2]	= ((WORD)( 2*line1[0] + 10*line1[1] +  4*line2[0] + 20*line2[1]))/36;
		y[2+sizeX*2]	= ((WORD)( 2*line1[1] +  4*line1[2] +  4*line2[1] +  8*line2[2]))/18;
		y[3+sizeX*2]	= ((WORD)( 4*line1[2] +  2*line1[3] +  8*line2[2] +  4*line2[3]))/18;

		//Line 3
		y[sizeX*3]	= ((WORD)( 4*line2[0]               +  2*line3[0]))/6;
		y[1+sizeX*3]	= ((WORD)( 4*line2[0] + 20*line2[1] +  2*line3[0] + 10*line3[1]))/36;
		y[2+sizeX*3]	= ((WORD)( 4*line2[1] +  8*line2[2] +  2*line3[1] +  4*line3[2]))/18;
		y[3+sizeX*3]	= ((WORD)( 8*line2[2] +  4*line2[3] +  4*line3[2] +  2*line3[3]))/18;

		//Line 4
		y[sizeX*4]	= ((WORD)( 5*line3[0]               +   line4[0]))/6;
		y[1+sizeX*4]	= ((WORD)( 5*line3[0] + 25*line3[1] +   line4[0] + 5*line4[1]))/36;
		y[2+sizeX*4]	= ((WORD)( 5*line3[1] + 10*line3[2] +   line4[1] + 2*line4[2]))/18;
		y[3+sizeX*4]	= ((WORD)(10*line3[2] +  5*line3[3] + 2*line4[2] +   line4[3]))/18;

		//Line 5
		y[sizeX*5]	= line4[0];
		y[1+sizeX*5]	= ((WORD)(  line4[0] + 5*line4[1]))/6;
		y[2+sizeX*5]	= ((WORD)(  line4[1] + 2*line4[2]))/3;
		y[3+sizeX*5]	= ((WORD)(2*line4[2] +   line4[3]))/3;
	}else if (num==5){

		//Line 0
		y[0]	= line0[0];
		y[1]	= ((WORD)(  line0[0] + 5*line0[1]))/6;
		y[2]	= ((WORD)(  line0[1] + 2*line0[2]))/3;
		y[3]	= ((WORD)(2*line0[2] +   line0[3]))/3;
		y[4]	= ((WORD)(5*line0[3] +   line0[4]))/6;
		
		//Line 1
		y[sizeX]	= ((WORD)(   line0[0]              +  5*line1[0]))/6;
		y[1+sizeX]	= ((WORD)(   line0[0] + 5*line0[1] +  5*line1[0] + 25*line1[1]))/36;
		y[2+sizeX]	= ((WORD)(   line0[1] + 2*line0[2] +  5*line1[1] + 10*line1[2]))/18;
		y[3+sizeX]	= ((WORD)( 2*line0[2] +   line0[3] + 10*line1[2] +  5*line1[3]))/18;
		y[4+sizeX]	= ((WORD)( 5*line0[3] +   line0[4] + 25*line1[3] +  5*line1[4]))/36;

		//Line 2
		y[sizeX*2]	= ((WORD)( 2*line1[0]               +  4*line2[0]))/6;
		y[1+sizeX*2]	= ((WORD)( 2*line1[0] + 10*line1[1] +  4*line2[0] + 20*line2[1]))/36;
		y[2+sizeX*2]	= ((WORD)( 2*line1[1] +  4*line1[2] +  4*line2[1] +  8*line2[2]))/18;
		y[3+sizeX*2]	= ((WORD)( 4*line1[2] +  2*line1[3] +  8*line2[2] +  4*line2[3]))/18;
		y[4+sizeX*2]	= ((WORD)(10*line1[3] +  2*line1[4] + 20*line2[3] +  4*line2[4]))/36;

		//Line 3
		y[sizeX*3]	= ((WORD)( 4*line2[0]               +  2*line3[0]))/6;
		y[1+sizeX*3]	= ((WORD)( 4*line2[0] + 20*line2[1] +  2*line3[0] + 10*line3[1]))/36;
		y[2+sizeX*3]	= ((WORD)( 4*line2[1] +  8*line2[2] +  2*line3[1] +  4*line3[2]))/18;
		y[3+sizeX*3]	= ((WORD)( 8*line2[2] +  4*line2[3] +  4*line3[2] +  2*line3[3]))/18;
		y[4+sizeX*3]	= ((WORD)(20*line2[3] +  4*line2[4] + 10*line3[3] +  2*line3[4]))/36;

		//Line 4
		y[sizeX*4]	= ((WORD)( 5*line3[0]               +   line4[0]))/6;
		y[1+sizeX*4]	= ((WORD)( 5*line3[0] + 25*line3[1] +   line4[0] + 5*line4[1]))/36;
		y[2+sizeX*4]	= ((WORD)( 5*line3[1] + 10*line3[2] +   line4[1] + 2*line4[2]))/18;
		y[3+sizeX*4]	= ((WORD)(10*line3[2] +  5*line3[3] + 2*line4[2] +   line4[3]))/18;
		y[4+sizeX*4]	= ((WORD)(25*line3[3] +  5*line3[4] + 5*line4[3] +   line4[4]))/36;

		//Line 5
		y[sizeX*5]	= line4[0];
		y[1+sizeX*5]	= ((WORD)(  line4[0] + 5*line4[1]))/6;
		y[2+sizeX*5]	= ((WORD)(  line4[1] + 2*line4[2]))/3;
		y[3+sizeX*5]	= ((WORD)(2*line4[2] +   line4[3]))/3;
		y[4+sizeX*5]	= ((WORD)(5*line4[3] + 1*line4[4]))/6;
	}

}
inline void ZoomBox5x6(BYTE *y,BYTE *pY,int sizeX)
{
	BYTE * line0 = pY;
	BYTE * line1 = pY + sizeX;
	BYTE * line2 = pY + sizeX*2;
	BYTE * line3 = pY + sizeX*3;
	BYTE * line4 = pY + sizeX*4;

	//Line 0
	*(y++)	= pY[0];
	*(y++)	= ((WORD)(  line0[0] + 5*line0[1]))/6;
	*(y++)	= ((WORD)(  line0[1] + 2*line0[2]))/3;
	*(y++)	= ((WORD)(2*line0[2] +   line0[3]))/3;
	*(y++)	= ((WORD)(5*line0[3] +   line0[4]))/6;
	*(y++)	= pY[4];
	y+=sizeX-6;
	
	//Line 1
	*(y++)	= ((WORD)(   line0[0]              +  5*line1[0]))/6;
	*(y++)	= ((WORD)(   line0[0] + 5*line0[1] +  5*line1[0] + 25*line1[1]))/36;
	*(y++)	= ((WORD)(   line0[1] + 2*line0[2] +  5*line1[1] + 10*line1[2]))/18;
	*(y++)	= ((WORD)( 2*line0[2] +   line0[3] + 10*line1[2] +  5*line1[3]))/18;
	*(y++)	= ((WORD)( 5*line0[3] +   line0[4] + 25*line1[3] +  5*line1[4]))/36;
	*(y++)	= ((WORD)(   line0[4]              +  5*line1[4]))/6;
	y+=sizeX-6;

	//Line 2
	*(y++)	= ((WORD)( 2*line1[0]               +  4*line2[0]))/6;
	*(y++)	= ((WORD)( 2*line1[0] + 10*line1[1] +  4*line2[0] + 20*line2[1]))/36;
	*(y++)	= ((WORD)( 2*line1[1] +  4*line1[2] +  4*line2[1] +  8*line2[2]))/18;
	*(y++)	= ((WORD)( 4*line1[2] +  2*line1[3] +  8*line2[2] +  4*line2[3]))/18;
	*(y++)	= ((WORD)(10*line1[3] +  2*line1[4] + 20*line2[3] +  4*line2[4]))/36;
	*(y++)	= ((WORD)( 2*line1[4]               +  4*line2[4]))/6;
	y+=sizeX-6;

	//Line 3
	*(y++)	= ((WORD)( 4*line2[0]               +  2*line3[0]))/6;
	*(y++)	= ((WORD)( 4*line2[0] + 20*line2[1] +  2*line3[0] + 10*line3[1]))/36;
	*(y++)	= ((WORD)( 4*line2[1] +  8*line2[2] +  2*line3[1] +  4*line3[2]))/18;
	*(y++)	= ((WORD)( 8*line2[2] +  4*line2[3] +  4*line3[2] +  2*line3[3]))/18;
	*(y++)	= ((WORD)(20*line2[3] +  4*line2[4] + 10*line3[3] +  2*line3[4]))/36;
	*(y++)	= ((WORD)( 4*line2[4]               +  2*line3[4]))/6;
	y+=sizeX-6;

	//Line 4
	*(y++)	= ((WORD)( 5*line3[0]               +   line4[0]))/6;
	*(y++)	= ((WORD)( 5*line3[0] + 25*line3[1] +   line4[0] + 5*line4[1]))/36;
	*(y++)	= ((WORD)( 5*line3[1] + 10*line3[2] +   line4[1] + 2*line4[2]))/18;
	*(y++)	= ((WORD)(10*line3[2] +  5*line3[3] + 2*line4[2] +   line4[3]))/18;
	*(y++)	= ((WORD)(25*line3[3] +  5*line3[4] + 5*line4[3] +   line4[4]))/36;
	*(y++)	= ((WORD)( 5*line3[4]               +   line4[4]))/6;
	y+=sizeX-6;

	//Line 5
	*(y++)	= line4[0];
	*(y++)	= ((WORD)(  line4[0] + 5*line4[1]))/6;
	*(y++)	= ((WORD)(  line4[1] + 2*line4[2]))/3;
	*(y++)	= ((WORD)(2*line4[2] +   line4[3]))/3;
	*(y++)	= ((WORD)(5*line4[3] + 1*line4[4]))/6;
	*(y++)	= line4[4];
}

inline void MixLine5to6(BYTE *y,BYTE *pY1,BYTE *pY2,int sizeX)
{
	register j;

	//Copiamos el primer
	for (j = 0; j<sizeX; j++)
		*(y++) = ((WORD)(*(pY1++)+*(pY2++)))>>1;
}

void UnborderAndZoom(BYTE *out,BYTE *in,int sizeX,int sizeY)
{
	int numpixels = sizeX*sizeY;
	int height = sizeY*5/6;
	//Mantenemos el aspect ratio
	int width  = 2*((sizeX*5/6)/2);
	int left   = (sizeX-width)/2;
	int top    = (sizeY-height)/2;

	//PUnteros de entrada
	BYTE *pY = (in	) 		+ (left + top*sizeX);
	BYTE *pU = (in + numpixels) 	+ (left/2+top*sizeX/4); 
	BYTE *pV = (in + numpixels*5/4)	+ (left/2+top*sizeX/4);

	//PUnteros de salida
	BYTE *y  = out;
	BYTE *u  = out + numpixels;
	BYTE *v  = out + numpixels*5/4;

	memset(y,0,numpixels*3/2);

	int i;
	int j;

	//Copiamos en bloques
	for (i = 0; i<height/5; i++)
	{
		for (j = 0; j<width/5;j++)
			ZoomBox5x6(y+6*j,pY+5*j,sizeX);
		ZoomPixels5x6(y+6*j,pY+5*j,sizeX,sizeX-j*6);
		y +=sizeX*6;
		pY+=sizeX*5;
	}
	for (i = 0; i<height/10; i++)
	{
		for (j = 0; j<width/10;j++)
		{
			ZoomBox5x6(u+6*j,pU+5*j,sizeX/2);
			ZoomBox5x6(v+6*j,pV+5*j,sizeX/2);
		}
		ZoomPixels5x6(u+6*j,pU+5*j,sizeX/2,sizeX/2-j*6);
		ZoomPixels5x6(v+6*j,pV+5*j,sizeX/2,sizeX/2-j*6);

		u +=sizeX*3;
		pU+=sizeX*5/2;
		v +=sizeX*3;
		pV+=sizeX*5/2;
	}
}


static void shrink22(BYTE *dst, int dst_wrap, BYTE *src, int src_wrap, int width, int height)
{
	int w;
	BYTE *s1, *s2;
	BYTE *d;
	for(;height > 0; height--) 
	{
		s1 = src;
		s2 = s1 + src_wrap;
		d = dst;
		for(w = width;w >= 4; w-=4) 
		{
			d[0] = (s1[0] + s1[1] + s2[0] + s2[1] + 2) >> 2;
			d[1] = (s1[2] + s1[3] + s2[2] + s2[3] + 2) >> 2;
			d[2] = (s1[4] + s1[5] + s2[4] + s2[5] + 2) >> 2;
			d[3] = (s1[6] + s1[7] + s2[6] + s2[7] + 2) >> 2;
			s1 += 8;
			s2 += 8;
			d += 4;
		}
		for(;w > 0; w--) 
		{
			d[0] = (s1[0] + s1[1] + s2[0] + s2[1] + 2) >> 2;
			s1 += 2;
			s2 += 2;
			d++;
		}
		src += 2 * src_wrap;
		dst += dst_wrap;
	}
}



static void half_yuv(BYTE *out,int ow,int oh,BYTE *in,int w,int h)
{
	shrink22(out		,ow	,in		,w	,ow,oh);
	shrink22(out+ow*oh	,ow/2	,in+w*h		,w/2	,ow/2,oh/2);
	shrink22(out+ow*oh*5/4	,ow/2	,in+w*h*5/4	,w/2	,ow/2,oh/2);
}
static void double_yuv(BYTE *in,BYTE *out,int w,int h)
{
}

void zoom_yuv(BYTE *out,int  zw,int zh,BYTE *in,int w,int h)
{
	//Calculamos la relacion de aspecto
	if ((w==2*zw) && (h==2*zh))
		half_yuv(out,zw,zh,in,w,h);
	else if ((2*w==zw) && (2*h==zh))
		double_yuv(in,out,w,h);
	else if ((w==zw) && (h==zh))
		memcpy(out,in,w*h*3/2);
}
