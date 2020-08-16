#include "lmRtmp.h"

#define TEST_FILE 1

#ifdef TEST_FILE
int main(int argc, char *argv[])
{
	RTMP  rt;
	char *url = "rtmp://192.168.1.104/live";
	rp_netinf_t rtmpnet;
	
	rtmpnet.serurl = url;
	rtmpnet.rtmp_m = &rt;
	if(lm_crtmp_create(&rtmpnet) < 0)
	{
		printf("Create lm rtmp failed, exit\n");
		return -1;
	}

	char* videof = "./nat.h264";
	char* audiof = "./8k_1_16.g711a";
	rtmp_media_t rpm;
	rpm.type = STTY_AV_F;
	rpm.fd_m.vfile = videof; 
	rpm.fd_m.afile = audiof; 
	rpm.net_m = rtmpnet;
	rpm.rpv_m.vtype = MEDIA_TP_H264;
	rpm.rpv_m.width = MEDIA_TP_H264;
	rpm.rpv_m.hight = MEDIA_TP_H264;
	rpm.rpv_m.fps   = MEDIA_TP_H264;
	
	rpm.rpa_m.athr     = NULL;
	rpm.rpa_m.atype    = MEDIA_TP_G711A;
	rpm.rpa_m.chns     = 1;
	rpm.rpa_m.sampling = 8000;
	rpm.rpa_m.frame_sz = 320;
	rpm.rpa_m.faacinf.objectType  = LOW;
	rpm.rpa_m.faacinf.mpegVersion = MPEG2;
	rpm.rpa_m.faacinf.pcmBitSize  = 16;
	
	lm_start_file_stream(&rpm);
	lm_crtmp_destroy(&rtmpnet);

	return 0;
}
#else

int main(int argc, char *argv[])
{
	RTMP  rt;
	char *url = "xxxxx";
	rp_netinf_t rtmpnet;
	
	rtmpnet.serurl = url;
	rtmpnet.rtmp_m = &rt;
	lm_crtmp_create(&rtmpnet);

	rtmp_media_t rpm;
	rpm.type = STTY_AV_L;
	rpm.net_m = &rtmpnet;
	rpm.rpv_m.vtype = MEDIA_TP_H264;
	rpm.rpv_m.width = MEDIA_TP_H264;
	rpm.rpv_m.hight = MEDIA_TP_H264;
	rpm.rpv_m.fps   = MEDIA_TP_H264;
	
	rpm.rpa_m.athr     = NULL;
	rpm.rpa_m.atype    = MEDIA_TP_G711A;
	rpm.rpa_m.chns     = 1;
	rpm.rpa_m.sampling = 8000;
	rpm.rpa_m.frame_sz = 320;
	rpm.rpa_m.faacinf.objectType  = LOW;
	rpm.rpa_m.faacinf.mpegVersion = MPEG2;
	rpm.rpa_m.faacinf.pcmBitSize  = 16;
	
	lm_start_live_stream(&rpm);

	rp_pack_t rpk;
	char temp1[PPS_PSP_SZ], temp2[PPS_PSP_SZ];
	sps_pps_t spt;
	spt.pps_data = temp1;
	spt.sps_data = temp2;
	rpk.sppack = &spt;
	
	RTMPPacket vpacket, apacket;
	rpk.vpack = vpacket;
	rpk.vpack = apacket;

	char *v_frame, int frame_sz;
	char *aacframe, int frame_asz;
	ullong nTimestamp = 0;

	while(1)
	{
		if()
		{
			//get video frame from video queue
				
			//pack video rtmp      pack and send to rtmp server.
			lm_push_video_data(rpk, v_frame, frame_sz, MEDIA_TP_H264, nTimestamp);
		}

		if()
		{
			//get audio frame from audio queue
			
			//if isn't aac audio stream, convert to aac stream.
			lm_conv2_aac_stream();
			//pack audio rtmp      pack and send to rtmp server.
			lm_push_audio_data(rpk, aacframe, frame_asz);
		}
	}
	
	lm_crtmp_destroy(rtmpnet);

	return 0;
}
#endif
