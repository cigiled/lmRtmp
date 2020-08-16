#include "lmRtmp.h"

//pravate function
fty_t ftm[ ] ={ {"jpeg",  MEDIA_TP_JPEG},  {"h264", MEDIA_TP_H264}, {"h265",  MEDIA_TP_H265},
				{"aac",   MEDIA_TP_AAC},   {"pcm",  MEDIA_TP_PCM},  {"g711a", MEDIA_TP_G711A}, 
				{"G711u", MEDIA_TP_G711U}, {"G726", MEDIA_TP_G726}, {"opus",  MEDIA_TP_OPUS}			  
		      }; 

inline int find_audio_flag(uint fmt, char *data, int *size);
int send_video_rtmp_pack(rp_pack_t *rpk, int size, int nTimestamp);

void print_tnt(char *buf, int len, const char *comment)
{
	int i = 0;
	printf("\r\n%s start buf:%p len:%d \r\n", comment, buf, len);
	for(i = 0; i < len; i++)
	{
    	if(i%16 == 0)
		printf("\r\n");
    	 printf("%02x ", buf[i] & 0xff); //&0xff 是为了防止64位平台中打印0xffffffa0 这样的，实际打印0xa0即可
	}
    printf("\n");
}


int check_para(rtmp_media_t *rpm, char bfile)
{
	finf_t	   	*fdm  = &rpm->fd_m; 
	rp_netinf_t *netm = &rpm->net_m;
	rp_video_t  *rpvm = &rpm->rpv_m; 
	rp_audio_t  *rpam = &rpm->rpa_m;
	
	// check RTMP struct obj.
	PRT_COND(!netm->rtmp_m, "[rtmp_m] struct obj don't create, is NULL, exit !\n");
	//  video stream enc type. 
	PRT_COND(rpvm->vtype < MEDIA_TP_JPEG || rpvm->vtype > MEDIA_TP_H265, "video type set error, exit !\n");	
	//  video stream width . 
	PRT_COND(rpvm->width < 0 || rpvm->width > MAX_V_WIDTH, "video width set out of range[0 - MAX_V_WIDTH], exit !\n");
	//  video stream hignt. 
	PRT_COND(rpvm->hight < 0 || rpvm->hight > MAX_V_HIGHT, "video hight set out of range[0 - MAX_V_HIGHT], exit !\n");
	//  video stream fps. 
	PRT_COND(rpvm->fps < 0 || rpvm->fps > MAX_V_FPS, "video fps set out of range[0 - MAX_V_FPS], exit !\n");

	if(rpm->type == STTY_AV_F || rpm->type == STTY_AV_L)
	{//if it have audio stream.
		//  audio stream fps. 
		PRT_COND(rpam->atype < MEDIA_TP_AAC || rpam->atype > MEDIA_TP_OPUS, "audio type set error, exit !\n");	
		//  audio stream fps. 
		PRT_COND(rpam->sampling < 0 || rpam->sampling > MAX_A_SAMPLING, "video type set set out of range[0 - MAX_A_SAMPLING], exit !\n");
		//  audio stream fps. 
		PRT_COND(rpam->chns < 0 || rpam->chns > 2, "video type set out of range[0 - 2], exit !\n");
	}
	
	if(bfile)
	{
		// video file name check.
		PRT_COND(!fdm->vfile, "Input file is empty, exit !\n");
		
		if(!strcasestr(fdm->vfile, ftm[rpvm->vtype].fty))
		{
			printf("Video file check fialed, please use example: xx.h264 or xx.h265\n");
			return -1;
		}

		if(rpm->type == STTY_AV_F)
		{//if it have audio stream.
			if(fdm->afile)
			{
				if(!strcasestr(fdm->afile, ftm[rpam->atype].fty))
				{
					printf("Audio file check fialed, please use example: xx.g711a or xx.aac\n");
					return -1;
				}
			}
		}
	}
	
	return 0;
}

int open_file(finf_t *fin)
{
	if(fin->vfile)
	{
		fin->vfd = fopen(fin->vfile, "r+");
		if(fin->vfd < 0)
		{
			perror("Fopen vidoe file failed, error:\n");
			return -1;
		}

		return 0;
	}

	if(fin->afile)
	{
		fin->afd = fopen(fin->afile, "r+");
		if(fin->afd < 0)
		{
			perror("Fopen audio file failed, error:\n");
			return -1;
		}

		return 0;
	}
	
	return -1;
} 



//======================================================================
////                    faac lib use interface.
//======================================================================
#define FAAC_MODULE
#ifdef FAAC_MODULE

#if 0
void *faac_pthr_proc(void *argv)
{
	FILE *fd = (FILE *)argv;
	int ret = 0;
	
	while(1)
	{
		ret = fread(,1,fd);
		faac_conver_stream();
	}
}
#endif

int init_faac(rp_audio_t *rpa)
{  
	int ret, framesize = 0;
	faacEncConfigurationPtr pConfiguration;
	faac_inf_t *fainf = &rpa->faacinf;

	/*open FAAC engine*/
	fainf->Encoder = faacEncOpen(rpa->sampling, rpa->chns, &fainf->InputSamples, &fainf->MaxOutputBytes);
	PRT_COND(fainf->Encoder == NULL, "failed to call faacEncOpen !\n")

	printf("nInputSamples == %ld, nMaxOutput == %ld !\n", fainf->InputSamples, fainf->MaxOutputBytes);
	/*get current encoding configuration*/
	pConfiguration = faacEncGetCurrentConfiguration(fainf->Encoder);
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;

	/*0 - raw; 1 - ADTS*/
	pConfiguration->useTns = 0;
	pConfiguration->outputFormat  = 1;
	pConfiguration->aacObjectType = fainf->objectType;/*此处输出格式很重要，否者在海思平台转码会很耗时*/
	pConfiguration->quantqual     = 20;/*默认为100 ，越小编码效率越高，不过质量也越差，感觉不明显*/
	pConfiguration->mpegVersion   = fainf->mpegVersion;

	fainf->pcmBuffsz = (fainf->InputSamples * (fainf->pcmBitSize/8));
	find_audio_flag(rpa->atype, NULL, &framesize); //just for getting decode frame size.
	rpa->frame_sz = framesize *2; //get pcm size.
 	fainf->pcmdata   =(char *)malloc(fainf->pcmBuffsz + rpa->frame_sz); // 加 frame_sz是为了...
	PRT_COND(!fainf->pcmdata, "Malloc pcmdata mem failed !")

	/*set encoding configuretion*/
	ret = faacEncSetConfiguration(fainf->Encoder, pConfiguration);
	return ret;
}

int start_faac_module(rp_audio_t *rpa, FILE* afd, uchar bpthread)
{
	int ret =0;
	if(bpthread == 0x01)
	{
		//ret = pthread_create(rpa->athr, NULL, faac_pthr_proc, afd);
	}

	ret = init_faac(rpa);
	return ret;
}

int faac_destroy(faac_inf_t *faccinf)
{
	faacEncClose(faccinf->Encoder);

	return 0;
}

inline int faac_conver_stream(rp_audio_t      *au, char *src, int src_sz, char *dst, int *dst_sz)
{
	int pcmsz = 0, ret, nTmp;
	faac_inf_t *finf = &au->faacinf;
	
	// decode to pcm stream.
	if(au->atype != MEDIA_TP_PCM)
	{
		pcmsz = aprt[au->atype - MEDIA_TP_PCM].decode(src, src_sz, finf->pcmdata + finf->currCount, &ret);
		finf->currCount += pcmsz;	
	}
	else
	{ //pcm 直接 累加 至 满足 finf->pcmBuffsz 个.
		finf->pos += src_sz;
		finf->currCount += src_sz;
	}
	
	if((finf->pcmBuffsz - finf->currCount) > pcmsz)
	{
		return 1;
	}

	//use faac lib to encode to be aac stream.
	ret = faacEncEncode(finf->Encoder, (int*)finf->pcmdata, finf->InputSamples, (uchar*)dst, finf->MaxOutputBytes);
	memmove(finf->pcmdata, &finf->pcmdata[finf->pcmBuffsz], finf->pcmBuffsz); //上面已经处理了pcmBuffsz,所以这里把剩下的移到前面去.
	nTmp = pcmsz - (finf->pcmBuffsz - finf->currCount);//本次decode后剩下的为 encaac的数据.
	finf->currCount  = 0;
	finf->currCount += nTmp;
	finf->pos = finf->pcmdata;
	*dst_sz = ret;
	
	return ret;
}

#endif

//======================================================================
////                    other interface.
//======================================================================
inline int check_nalu_type(char *data, uint type)
{
	if(type == MEDIA_TP_H264)
	{
		if( data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1)
			return 4;
		else if(data[0] == 0 && data[1] == 0 && data[2] == 1)
			return 3;
	}
	else if(type == MEDIA_TP_H265)
	{

	}

	return 0;
}

int get_file_remsize(FILE *fd)
{
	uint currpos, endpos;
	
	currpos =  ftell(fd);
	fseek(fd, 0L, SEEK_END);
	endpos = ftell(fd) - currpos;
	fseek(fd, currpos, SEEK_SET);
	
	return endpos;
}


inline int get_onenalu_data(uint enc_ty, FILE *fd, char *frame0, int *frame_sz0, char *buff0,  uint *nal_sz)
{
	PRT_COND(!frame0 || !fd, "Input frame0 or fd is NULL !");
	PRT_COND(!buff0, "vidoe mem is NULL !");

	int sqe = 0;
	static int l_nalsz = 0;
	static int p   = 0;
	static int len = 0;
	static int remain = 0;
	static int file_sz = 0;
	static char end_flag = 1;
	static char *start = NULL;
	
	while(1)
	{
		if(end_flag)
		{
			p = 0;
			memset(buff0, 0, MAX_VIDEO_BUFF);
			if((len = fread(buff0, 1, MAX_VIDEO_BUFF, fd)) <= 0)
			{
				return -2; //file over.
			}
		}

		if(check_nalu_type(&buff0[p], enc_ty))
		{				
			(buff0[p+2] == 1)?(sqe = 3):(sqe = 4);
			if(!start)
			{ //首次检测到帧.									
				start    = &buff0[p];
				p	    += sqe;
				l_nalsz  = sqe;
				end_flag = 0;
				continue;
			}
			else
			{
				if(remain > 0)
				{
					memcpy(frame0 + remain, &buff0[0], p); //加上 上次不完整部分数据.
					*frame_sz0 = p + remain;
					remain = 0; //上次read 剩下的不完整帧.
				}
				else
				{
					*frame_sz0 = &buff0[p] - start;
					memcpy(frame0, start, *frame_sz0);
				}
				
				start   = &buff0[p];
				p	   += sqe;
				*nal_sz = l_nalsz;
				l_nalsz  = sqe;
				end_flag = 0;
				
				return 1;
			} 
		}
		else if( len == p)
		{//存下这次read帧中, 剩下的不完整帧数据.
			remain = &buff0[p] - start;	
			if(remain > 0)
			{
				memcpy(frame0, start, remain);
				if(get_file_remsize(fd) == 0)
				{ //写最后一帧数据.						
					return 1;
				}
			}

			file_sz += len;
			end_flag = 1;
			continue;
		}
		
		p++;
	}

	return 0;
}

int send_SpsPps_pack(sps_pps_t *spt1, RTMP *rtmp)
{
	int i = 0;
	rp_pack_t rpk;
	RTMPPacket *packet = (RTMPPacket *)malloc(RTMP_HEAD_SZ+1024);

	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet, 0, RTMP_HEAD_SZ + 1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SZ;
	uchar *body = (uchar *)packet->m_body;
	
	body[i++] = 0x17;
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = spt1->sps_data[1];
	body[i++] = spt1->sps_data[2];
	body[i++] = spt1->sps_data[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++] = 0xe1;
	body[i++] = (spt1->sps_len >> 8) & 0xff;
	body[i++] = spt1->sps_len & 0xff;
	memcpy(&body[i], spt1->sps_data, spt1->sps_len);
	i += spt1->sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (spt1->pps_len >> 8) & 0xff;
	body[i++] = (spt1->pps_len) & 0xff;
	memcpy(&body[i], spt1->pps_data, spt1->pps_len);
	i += spt1->pps_len;

	rpk.vpack = packet;
	rpk.rtmp = rtmp;

	print_tnt(packet->m_body, i, "send sps pps rtmp pack:");

	send_video_rtmp_pack(&rpk, i, 0);
	free(packet); //释放内存

	return 0;
}

/* +9 byte for frame info.*/
inline int fill_nalu_pack(uint fmt, char *frame, int framesz, char *body, rp_pack_t *rpk)
{
	char *pNalu = frame;
	int i = 0, iskeyframe = 0x00;
	char type = 0;
	//只记关键帧
	if(fmt == MEDIA_TP_H264)
	{
		type = pNalu[0] & 0x1F;
		if(type == 0x05)
			iskeyframe = 0x01;
		else if(type == 0x07)
		{
			print_tnt(frame, framesz, "sps:");
			memcpy(rpk->sppack->sps_data, frame, framesz);
			rpk->sppack->sps_len = framesz;
			return 1;
		}
		else if(type == 0x08)
		{
			print_tnt(frame, framesz, "pps:");
			memcpy(rpk->sppack->pps_data, frame, framesz);
			rpk->sppack->pps_len = framesz;
			return 1;
		}	
	}
	else if (fmt == MEDIA_TP_H265)
	{
		type = (pNalu[0] & 0x7E)>>1;
		if(type == 19)		
			iskeyframe = 0x01;
	}

	if(iskeyframe == 0x01)
	{  
		body[i++] = 0x17;// 1:Iframe  7:AVC  
		send_SpsPps_pack(rpk->sppack, rpk->rtmp);
	}
	else
	{  
		body[i++] = 0x27;// 2:Pframe  7:AVC    
	}  

	body[i++] = 0x01;// AVC NALU   
	body[i++] = 0x00;  
	body[i++] = 0x00;   
	body[i++] = 0x00;  

	// NALU size   
	body[i++] = framesz>>24 &0xff;  
	body[i++] = framesz>>16 &0xff;  
	body[i++] = framesz>>8 &0xff;  
	body[i++] = framesz&0xff;
	return 0;
}

int send_audio_rtmp_pack(rp_pack_t *rpk, char *src, int size, int nTimestamp, int first)
{
	char *body = src;
	rpk->apack->m_headerType = RTMP_PACKET_SIZE_LARGE;
	rpk->apack->m_nChannel   = 0x04;
	rpk->apack->m_hasAbsTimestamp = 0;
	rpk->apack->m_body = body;
	*(body++) = 0xAF;

	if(!first)
	{
		 rpk->apack->m_packetType = RTMP_PACKET_SIZE_MEDIUM ;
		 rpk->apack->m_nBodySize  = size - 5;
		 *(body++) = 0x01; // memcpy(body, data + 7, size - 7);	
	}
	else
	{
		 rpk->apack->m_packetType = RTMP_PACKET_TYPE_AUDIO;
		 rpk->apack->m_nBodySize  = size + 2;
		 *(body++) = 0x00;// memcpy(body, data, size);
	}	

	//send rtmp pack.
	if (RTMP_IsConnected(rpk->rtmp))
	{
		int ret;
		ret = RTMP_SendPacket(rpk->rtmp,rpk->apack,TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}

	return 0;
}

int get_audio_data(uint enc_ty, FILE *fd, char *frame1, int *frame_sz1, char *buff1)
{
	PRT_COND(!frame1 || !fd, "Input frame1 or fd is NULL !");
	PRT_COND(!buff1, "audio mem is NULL !");
	
	static int i = 0;
	static int re_sz = 0;
	static int a_remain = 0;
	static int frame_len = 0;
	static char end_flag = 1;
	static uchar  fullframe = 1;

	if(end_flag)
	{
		memset(buff1, 0, MAX_AUDIO_BUFF);
		re_sz = fread(buff1, 1, MAX_AUDIO_BUFF, fd);
		PRT_CONDX(re_sz<= 0, "read over !", -2);
		end_flag = 0;
	}
			
	while(1)
	{				
		if(find_audio_flag(enc_ty, &buff1[i], &frame_len) > 0)
		{
			if(re_sz >= frame_len)
			{
				memcpy(frame1, &buff1[i], frame_len);
				i += frame_len;    //加一帧的长度.
				re_sz -= frame_len;//剩下的帧数据...
				
				return 0;
			}
			else
			{ //剩下帧不完整.
				memcpy(frame1, &buff1[i], re_sz); //保存剩下的帧数.
				a_remain = frame_len - re_sz;
				
				end_flag  = fullframe = 0;
				frame_len = i = re_sz = 0;
				return -1;
			}
		}
		else
		{
			if(a_remain > 0)
			{//处理上次不完整的帧.
				if(!fullframe)
				{
					fullframe = 1;
					memcpy(frame1 , &buff1[i], a_remain);
					i += a_remain;   
					re_sz -= a_remain;
					a_remain = 0;
					return 0;
				}
			}
		}
		i++;
	} 

	return 0;
}

inline int find_audio_flag(uint fmt, char *data, int *size)
{	
	if(fmt == MEDIA_TP_AAC)
	{
		//Sync words: 0xfff;
		if(( (data[0]&0xff) == 0xff) && ((data[1]&0xf0) == 0xf0) )
		{
			//frame_length: 13 bit
			*size |= (data[3] & 0x03) <<11; //low    2 bit
			*size |= (data[4] & 0xff) <<3;	//middle 8 bit
			*size |= (data[5] & 0xe0) >>5;	//hight  3 bit
			
			return 1;
		}
	}
	else
	{
		if(fmt == MEDIA_TP_G711A || fmt == MEDIA_TP_G711U || fmt == MEDIA_TP_G726)
			*size = 320;
		else if(fmt == MEDIA_TP_PCM)
			*size = 3265;
		
		return 1;
	}
	
	return 0;
}

int send_video_rtmp_pack(rp_pack_t *rpk, int size, int nTimestamp)
{
	int nRet =0;
	//set rtmp pack.
	rpk->vpack->m_nChannel    = 0x04;
	rpk->vpack->m_nBodySize   = size;
	rpk->vpack->m_packetType  = RTMP_PACKET_TYPE_VIDEO; /*此处为类型有两种一种是音频,一种是视频*/
	rpk->vpack->m_headerType  = RTMP_PACKET_SIZE_LARGE;
	rpk->vpack->m_nTimeStamp  = nTimestamp;
	rpk->vpack->m_nInfoField2 = rpk->rtmp->m_stream_id;
	rpk->vpack->m_hasAbsTimestamp = 0;
	
	//send rtmp pack.
	if (RTMP_IsConnected(rpk->rtmp))
	{
		nRet = RTMP_SendPacket(rpk->rtmp,rpk->vpack,TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}

	return 0;
}

//为了减少fread的次数.这里每次读多帧到buff, 然后再解析出一帧来.
int send_rtmp_filestream(rtmp_media_t *rpm1)
{
	int frame_sz; uint nalsz, venc_ty;
	int time_v = 0, time_a = 1, ret = 0, times = 0;
	char video_end = 0, audio_end = 0;	

	char *v_buff  = malloc(MAX_VIDEO_BUFF);
	char *a_buff  = malloc(MAX_AUDIO_BUFF);

	RTMPPacket *vpacket, *apacket;
	vpacket = (RTMPPacket *)malloc(MAX_V_FAME_BUFF);
	apacket = (RTMPPacket *)malloc(MAX_A_FAME_BUFF);

	char *v_frame, *a_frame;
	v_frame = vpacket->m_body = (char *)vpacket + RTMP_HEAD_SZ;
	a_frame = apacket->m_body = (char *)apacket + RTMP_HEAD_SZ+2;

	rp_pack_t rpk;
	finf_t *hd = &rpm1->fd_m;
	rp_video_t *v = &rpm1->rpv_m;
	rp_audio_t *a = &rpm1->rpa_m;
	rpk.rtmp = rpm1->net_m.rtmp_m;
	rpk.vpack = vpacket;
	rpk.apack = apacket;

	uint vtick = 1000/v->fps;
	uint atick = 1024000/(a->sampling * a->chns);

	char temp1[PPS_PSP_SZ], temp2[PPS_PSP_SZ];
	sps_pps_t spt;
	spt.pps_data = temp1;
	spt.sps_data = temp2;
	rpk.sppack = &spt;

	char *atmp = NULL; //read audio file  tmp buff.
	int readsz = 0; //read audio file data size.
	if((rpm1->type == STTY_AV_F) && (rpm1->rpa_m.atype != MEDIA_TP_AAC))
	{
		if(rpm1->rpa_m.atype == MEDIA_TP_PCM)
		{
			a->faacinf.pos = a->faacinf.pcmdata;
			a->faacinf.currCount = 0;
		}
		else	
		{
			atmp = (char *)malloc(MAX_A_FAME_BUFF);
			a->faacinf.pos = atmp;
		}
	}
	
	venc_ty = v->vtype;
	
	while(1)
	{
		if((hd->vfd) && (!video_end))
		{ // video 
			if(time_v < time_a)
			{
				printf("----------->\n");
				frame_sz = nalsz = 0;
				get_onenalu_data(venc_ty, hd->vfd, &v_frame[9], &frame_sz, v_buff, &nalsz);
				frame_sz -= nalsz;
				ret = fill_nalu_pack(venc_ty, &v_frame[9+nalsz], frame_sz, v_frame, &rpk);
				if(ret == 1) // skip pps,sps.
					continue;
				
				ret = send_video_rtmp_pack(&rpk, frame_sz+9, time_v);
				PRT_COND(ret < 0, "Send rtmp video frame failed\n");
				
				if(ret == 2)
					video_end = 1;
				
				time_v +=vtick;
			}
		}
		
		if((hd->afd) && (!audio_end))
		{   //audio. get aac frame, if isnot aac format, will use faac lib to convert to aac streame and get aac frame.
			if(time_v > time_a)
			{
				printf("--+++++++++->\n");
				frame_sz = nalsz = 0;
				if(a->atype != MEDIA_TP_AAC)
				{
					get_audio_data(a->atype, hd->afd, a->faacinf.pos, &readsz, a_buff);
					faac_conver_stream(a, a->faacinf.pos, readsz, &a_frame[9], &frame_sz);
				}
				else
				{
					get_audio_data(a->atype, hd->afd, &a_frame[9], &frame_sz, a_buff);
				}
			
				ret = send_audio_rtmp_pack(&rpk, &a_frame[9-2], frame_sz+9, time_a, times);
				PRT_COND(ret < 0, "Send rtmp audio frame failed\n");
				times = 1; //for  distinguish audio data pack and Audio Sequence Heade pack.
				if(ret == 2)
					audio_end = 1;
					
				time_a += atick;
			}
		}

		if(audio_end && video_end)
			break;

		usleep(900);
	}

	if(atmp)
		free(atmp);
	free(vpacket);
	free(apacket);
	free(a_buff);
	free(a_frame);
	
	return 0;
}

/*
+data:
+18:
	RTMPPacket* packet;
	分配包内存和初始化,len为包体长度
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size);
*/

int start_file_stream_proc(rtmp_media_t *rpm1)
{
	PRT_COND(!rpm1, "[start_file_stream_proc]:input rpm1 is NULL, exit !\n");

	if((rpm1->type == STTY_AV_F) && (rpm1->rpa_m.atype != MEDIA_TP_AAC))
	{
		start_faac_module(&rpm1->rpa_m, rpm1->fd_m.afd, USE_PTHREAD);
	}

	send_rtmp_filestream(rpm1);

	//直播完一次后， 默认退出,销魂资源.
	if(rpm1->rpa_m.faacinf.pcmdata)
		free(rpm1->rpa_m.faacinf.pcmdata);
	faac_destroy(&rpm1->rpa_m.faacinf);
	return 0;
}



////////////////////////////////////////////////////////////////////////////
//                       public  interface 
////////////////////////////////////////////////////////////////////////////
#define PUBLIC_API
#ifdef PUBLIC_API
int lm_crtmp_create(rp_netinf_t *rtmpnet)
{
	PRT_COND(!rtmpnet, "[lm_crtmp_create]:input rtmpnet is NULL, exit !\n");
	PRT_COND(!rtmpnet->serurl, "[lm_crtmp_create]:input rtmp server url is NULL, exit !\n");

	RTMP *rtmp = rtmpnet->rtmp_m;
	
	rtmp= RTMP_Alloc();
	RTMP_Init(rtmp);
	
	if (RTMP_SetupURL(rtmp,(char*)rtmpnet->serurl) == FALSE) /*设置URL*/
	{
		RTMP_Free(rtmp);
		return -1;
	}
	
	RTMP_EnableWrite(rtmp);/*设置可写,即发布流,这个函数必须在连接前使用,否则无效*/
	if (RTMP_Connect(rtmp, NULL) == FALSE)  /*连接服务器*/
	{
		RTMP_Free(rtmp);
		return -1;
	} 

	if (RTMP_ConnectStream(rtmp,0) == FALSE) /*连接流*/
	{
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		return -1;
	}
	
	return 0;
}

int lm_crtmp_destroy(rp_netinf_t *rtmpnet)
{
	RTMP_Close(rtmpnet->rtmp_m);
	RTMP_Free(rtmpnet->rtmp_m);
	
	return 0;
}

int lm_start_file_stream(rtmp_media_t *rpm)
{
	int ret;
	ret = check_para(rpm, 1);
	PRT_COND(ret < 0, "[lm_start_file_stream]Input para check failed, exit !\n");

    open_file(&rpm->fd_m);
	start_file_stream_proc(rpm);
	return 0;
}

int lm_start_live_stream(rtmp_media_t *rpm)
{
	int ret;
	ret = check_para(rpm, 0);
	PRT_COND(ret < 0, "[lm_start_file_stream]Input para check failed, exit !\n");

	if( (rpm->type == STTY_AV_L) && (rpm->rpa_m.atype != MEDIA_TP_AAC) )
	{
		start_faac_module(&rpm->rpa_m, NULL, ~USE_PTHREAD);
	}
		
	return 0;
}

int lm_conv2_aac_stream(rp_audio_t      *au, char *src, int src_sz, char *dst, int *dst_sz)
{
	faac_conver_stream(au, src, src_sz, dst, dst_sz);
	return 0;
}

int lm_push_video_data(rp_pack_t *rpk, char *v_frame, int frame_sz, int venc_ty, int nTimestamp)
{
	int nalsz = check_nalu_type(v_frame, venc_ty);
	PRT_COND(nalsz <= 0, "Is not complete nalu frame !\n");
	
	int ret = fill_nalu_pack(venc_ty, &v_frame[9+nalsz], frame_sz, v_frame, rpk);
	if(ret == 1) // skip pps,sps.
		return 1;
	
	ret = send_video_rtmp_pack(rpk, frame_sz+9, nTimestamp);
	PRT_COND(ret < 0, "Send rtmp video frame failed\n");
	return 0;
}

//put aac audio, if isn't aac,  will convert audio stream befor call this inteface .
int lm_push_audio_data(rp_pack_t *rpk, char *aacframe, int frame_sz, int nTimestamp, int first)
{
	int ret = send_audio_rtmp_pack(rpk, aacframe, frame_sz+9, nTimestamp, first);
	PRT_COND(ret < 0, "Send rtmp audio frame failed\n");
	return 0;
}
#endif

