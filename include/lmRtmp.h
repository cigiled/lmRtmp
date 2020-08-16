#ifndef __LM_RTMP_H__
#define __LM_RTMP_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "lmRtmp_base.h"
#include "librtmp/rtmp.h"   
#include "librtmp/rtmp_sys.h"   
#include "librtmp/amf.h"  
#include "faac.h"

/**  rtmp push stream[ client ].
  *  note: destion by using the librtmp and the  libfacc, librtmp.
***/

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_V_WIDTH 	(2592)
#define MAX_V_HIGHT	 	(1944)
#define MAX_V_FPS 		(60)

#define MAX_VIDEO_BUFF  (1024*800)
#define MAX_V_FAME_BUFF (1024*144)
#define MAX_AUDIO_BUFF  (1024*80)
#define MAX_A_FAME_BUFF (1024*2)
#define MAX_FFLUSH_SZ   (1024*512)
#define MAX_FILE_SZ     (100)

#define MAX_A_SAMPLING 	(48000)
#define PPS_PSP_SZ		(720)

#define USE_PTHREAD     (0x01)

#define RTMP_HEAD_SZ  (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

//stream type:
typedef enum 
{
	STTY_V_F,   // only video file stream. [1 file]
	STTY_AV_F,  // video file stream + audio file stream. [2 files]
	STTY_MIX_F, // video + audio mix file stream. [1 file]
	STTY_V_L,   // video live stream.  [no file]
	STTY_AV_L,  // video + audio live stream.  [no file]
}stty_e;

typedef struct
{
	char *sps_data;
	char sps_len;
	char *pps_data;
	char pps_len;
}sps_pps_t;

typedef struct
{
	char *fty;
	char ty_id;
}fty_t;

typedef struct
{
	uint  objectType;
	uint  mpegVersion;
	ulong InputSamples;
	ulong MaxOutputBytes;
	faacEncHandle Encoder;
	ulong pcmBuffsz;
	ulong pcmBitSize;
	uint  currCount; // curr pcm count.
	char *pos;
	char *pcmdata;
}faac_inf_t;

//audio media inf
typedef struct
{
	mtp_e	   atype;
	char 	   chns;
	int 	   sampling;
	int 	   frame_sz;
	faac_inf_t faacinf;
	pthread_t *athr; //create audio pthread if need.
}rp_audio_t;

//video media inf
typedef struct
{
	mtp_e	vtype;
	uint 	width;
	uint 	hight;
	uint 	fps;
}rp_video_t;

//file handle.
typedef struct
{
	FILE *vfd;
	char *vfile;
	FILE *afd;
	char *afile;
}finf_t;

//rtmp server inf.
typedef struct
{
	char *serurl;
	RTMP *rtmp_m;
}rp_netinf_t;

//audio and vidoe media inf
typedef struct
{
	stty_e      type;
	finf_t	    fd_m;  //if live stream , will don't use it.
	rp_netinf_t net_m;
	rp_video_t  rpv_m;
	rp_audio_t  rpa_m;
}rtmp_media_t;

typedef struct
{
	RTMPPacket  *vpack;
	RTMPPacket  *apack;
	sps_pps_t   *sppack;
	RTMP *rtmp;
}rp_pack_t;

int lm_crtmp_create(rp_netinf_t *rtmpnet);
int lm_crtmp_destroy(rp_netinf_t *rtmpnet);

int lm_start_file_stream(rtmp_media_t *rpm);
int lm_start_live_stream(rtmp_media_t *rpm);

int lm_push_video_data(rp_pack_t *rpk, char *v_frame, int frame_sz, int venc_ty, int nTimestamp);
int lm_push_audio_data(rp_pack_t *rpk, char *aacframe, int frame_sz, int nTimestamp, int first);


int lm_conv2_aac_stream(rp_audio_t *au, char *src, int src_sz, char *dst, int *dst_sz);

#ifdef __cplusplus
}
#endif

#endif
