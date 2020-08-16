#include <stdio.h>
#include "lmRtmp_base.h"
#include "common_table.h"

 int g711a_decode(char *src, int src_sz, char *dst, int *dst_sz);
 int g711a_encode(char *src, int src_sz, char *dst, int *dst_sz);
 int g711u_decode(char *src, int src_sz, char *dst, int *dst_sz);
 int g711u_encode(char *src, int src_sz, char *dst, int *dst_sz);
 int g726_decode(char *src, int src_sz, char *dst, int *dst_sz);
 int g726_encode(char *src, int src_sz, char *dst, int *dst_sz);

 aproc_t aprt[] = {
			{MEDIA_TP_PCM, NULL, NULL},  
			{MEDIA_TP_G711A, g711a_decode, g711a_encode}, 
			{MEDIA_TP_G711U, g711u_decode, g711u_encode},
			{MEDIA_TP_G726,  g726_decode,  g726_encode}, 
			{MEDIA_TP_OPUS, NULL, NULL}, 
		  };

#define FAAC_LIB_USE
#ifdef FAAC_LIB_USE

#endif

#define G711A_FMT
#ifdef G711A_FMT
//note: 摘抄自 海思aac库"hisiG711aToAac\G711aToAac.c"文件.
int g711a_decode(char *src, int src_sz, char *dst, int *dst_sz)
{
	ushort *dst0 = (ushort *)dst;
	uchar *src0 = (uchar *)src;
	uint i = 0;
	int Ret = 0;

	if((NULL == dst) || (NULL == dst_sz) || (NULL == src) || (0 == src_sz))
	{
		return -1;
	}
	
	if (*dst_sz < 2 * src_sz)
	{
		return -2;
	}

	for(i = 0; i < src_sz; i++)
	{
		*(dst0) = alawtos16[*(src0)];
		dst0++;
		src0++;
	}

	*dst_sz = 2 * src_sz;
	Ret = 2 * src_sz;
	
	return Ret;
}

int g711a_encode(char *src, int src_sz, char *dst, int *dst_sz)
{
	char *dst0 = (char *)dst;
    short *src0 = (short *)src;
    uint i = 0;
    int Ret = 0;

    if((NULL == dst) || (NULL == dst_sz) || (NULL == src) || (0 == src_sz))
    {
        return -1;
    }
    
    if( *dst_sz < src_sz / 2)
    {
        return -2;
    }
    
    for(i = 0; i < src_sz / 2; i++)
    {
        short s = *(src0++);
        if(s >= 0)
        {
            *(dst0++) = alaw_encode[s / 16];
        }
        else
        {
            *(dst0++) = 0x7F & alaw_encode[s / -16];
        }
    }
    
    *dst_sz = src_sz / 2;
	Ret = src_sz / 2;
    
	return Ret;
}

#endif


#define G711U_FMT
#ifdef G711U_FMT

//note: 摘抄自网络..
#define	BIAS		(0x84)	/* Bias for linear code. */
#define	SIGN_BIT	(0x80)	/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)	/* Quantization field mask. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};
static int search(int val, short *table, int	size)
{
	int	i;

	for (i = 0; i < size; i++)
	{
		if (val <= *table++)
			return (i);
	}
	return (size);
}

static int ulaw2linear(uchar u_val)
{
	int	t;
	u_val = ~u_val;
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

uchar linear2ulaw(int pcm_val)
{
	int		mask;
	int		seg;
	uchar	uval;

	if (pcm_val < 0) 
   {
        pcm_val = BIAS - pcm_val;
        mask = 0x7F;
	} else {
        pcm_val += BIAS;
        mask = 0xFF;
	}

	seg = search(pcm_val, seg_end, 8);
	if (seg >= 8)
		return (0x7F ^ mask);
	else
   {
		uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
		return (uval ^ mask);
	}
}

int g711u_decode(char *src, int src_sz, char *dst, int *dst_sz)
{
	int i;
	int samples;
	uchar code;
	int sl;
	short *dsttmp = (short *)dst;

	for (samples = i = 0;;)
	{
		 if (i >= src_sz)
			break;
		 code = src[i++];
		 sl = ulaw2linear(code);
		 dsttmp[samples++] = (short) sl;
	}

	*dst_sz = samples*2;
	
	return *dst_sz;
}

int g711u_encode(char *src, int src_sz, char *dst, int *dst_sz)
{
    int i;
	short *srctmp = (short *)src;
    for (i = 0;  i < src_sz;  i++)
	{
        dst[i] = linear2ulaw(srctmp[i]);
    }
	
	*dst_sz = i;
    return *dst_sz;
}

#endif


#define G726_FMT
#ifdef G726_FMT

int g726_decode(char *src, int src_sz, char *dst, int *dst_sz)
{

	return 0;
}

int g726_encode(char *src, int src_sz, char *dst, int *dst_sz)
{

	return 0;
}

#endif

#define PCM_FMT
#ifdef G726_FMT



#endif

