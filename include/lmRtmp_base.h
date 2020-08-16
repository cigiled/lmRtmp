#ifndef __LM_RTMP_BASE_H__
#define __LM_RTMP_BASE_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned char 		uchar;
typedef unsigned short 		ushort;
typedef unsigned int  		uint;
typedef unsigned long 		ulong;
typedef unsigned long long  ullong;

#define NAME   "LmRtmp"

typedef enum __bool { false = 0, true = 1, } bool;

#define PRT_COND(val, str)  \
			{					\
				if((val) == true){		\
					printf(NAME":{%s}\n", str); \
					return -1;	\
				}	\
			}

#define PRT_CONDX(val, str, X)  \
				{					\
					if((val) == false){		\
						printf(NAME":{%s}\n", str); \
						return X;	\
					}	\
				}

//=========================================================
//media type.
typedef enum
{
	MEDIA_TP_JPEG,
	MEDIA_TP_H264,
	MEDIA_TP_H265,
	MEDIA_TP_AAC,	
	MEDIA_TP_PCM,
	MEDIA_TP_G711A,
	MEDIA_TP_G711U,
	MEDIA_TP_G726,
	MEDIA_TP_OPUS,
	MEDIA_TP_MAX,
}mtp_e;

typedef struct
{
	uchar fmt;
	int (*decode)(char *src, int src_sz, char *dst, int *dst_sz);
	int (*encode)(char *src, int src_sz, char *dst, int *dst_sz);
}aproc_t;


extern aproc_t aprt[];

#ifdef __cplusplus
}
#endif

#endif

