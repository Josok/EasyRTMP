/*
	Copyright (c) 2013-2014 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.EasyDarwin.org
*/
#include <stdio.h>
#include "EasyRTMPAPI.h"
#include <windows.h>
#include "hi_type.h"
#include "hi_net_dev_sdk.h"
#include "hi_net_dev_errors.h"

#define CHOST	"192.168.1.106"		//EasyCamera摄像机IP地址
#define CPORT	80					//EasyCamera摄像机端口
#define CNAME	"admin"
#define CPWORD	"admin"

HI_U32 u32Handle = 0;

#define SRTMP "rtmp://www.easydss.com/oflaDemo/sdk"

Easy_RTMP_Handle rtmpHandle = 0;

HI_S32 OnEventCallback(HI_U32 u32Handle, /* 句柄 */
                                HI_U32 u32Event,      /* 事件 */
                                HI_VOID* pUserData  /* 用户数据*/
                                )
{
	return HI_SUCCESS;
}


HI_S32 NETSDK_APICALL OnStreamCallback(HI_U32 u32Handle, /* 句柄 */
                                HI_U32 u32DataType,     /* 数据类型，视频或音频数据或音视频复合数据 */
                                HI_U8*  pu8Buffer,      /* 数据包含帧头 */
                                HI_U32 u32Length,      /* 数据长度 */
                                HI_VOID* pUserData    /* 用户数据*/
                                )
{

	HI_S_AVFrame* pstruAV = HI_NULL;
	HI_S_SysHeader* pstruSys = HI_NULL;
	bool bRet = false;

	if (u32DataType == HI_NET_DEV_AV_DATA)
	{
		pstruAV = (HI_S_AVFrame*)pu8Buffer;

		if (pstruAV->u32AVFrameFlag == HI_NET_DEV_VIDEO_FRAME_FLAG)
		{
			int spsOffset = 0, ppsOffset = 0, idrOffset = 0;

			if( pstruAV->u32VFrameType == HI_NET_DEV_VIDEO_FRAME_I)
			{
				char* pbuf = (char*)(pu8Buffer + sizeof(HI_S_AVFrame));

				for(int i=0; (i<pstruAV->u32AVFrameLen) && (pstruAV->u32AVFrameLen-i > 5); i++)
				{
					unsigned char naltype = ( (unsigned char)pbuf[i+4] & 0x1F);
					if (	spsOffset == 0	&&
							(unsigned char)pbuf[i]== 0x00 && 
							(unsigned char)pbuf[i+1]== 0x00 && 
							(unsigned char)pbuf[i+2] == 0x00 &&
							(unsigned char)pbuf[i+3] == 0x01 &&
							(naltype==0x07) )
					{
						spsOffset = i;
						continue;
					}

					if (	ppsOffset == 0	&&
							(unsigned char)pbuf[i]== 0x00 && 
							(unsigned char)pbuf[i+1]== 0x00 && 
							(unsigned char)pbuf[i+2] == 0x00 &&
							(unsigned char)pbuf[i+3] == 0x01 &&
							(naltype==0x08) )
					{
						ppsOffset = i;
						continue;
					}

					if (	idrOffset == 0	&&
					(unsigned char)pbuf[i]== 0x00 && 
					(unsigned char)pbuf[i+1]== 0x00 && 
					(unsigned char)pbuf[i+2] == 0x00 &&
					(unsigned char)pbuf[i+3] == 0x01 &&
					(naltype==0x05) )
					{
						idrOffset = i;
						break;
					}
				}

				/* 关键帧是SPS、PPS、IDR(均包含00 00 00 01)的组合 */
				if(rtmpHandle == 0)
				{
					rtmpHandle = EasyRTMP_Session_Create();

					bRet = EasyRTMP_Connect(rtmpHandle, SRTMP);
					if (!bRet)
					{
						printf("Fail to EasyRTMP_Connect ...\n");
					}

					bRet = EasyRTMP_InitMetadata(rtmpHandle, 
						(const char*)pu8Buffer+sizeof(HI_S_AVFrame)+spsOffset+4,
						ppsOffset - spsOffset -4,
						(const char*)pu8Buffer+sizeof(HI_S_AVFrame)+ppsOffset+4,
						idrOffset - ppsOffset -4,
						25, 8000);
					if (bRet)
					{
						printf("Fail to InitMetadata ...\n");
					}
				}
				bRet = EasyRTMP_SendH264Packet(rtmpHandle, 
					(unsigned char*)pu8Buffer+sizeof(HI_S_AVFrame) + idrOffset + 4,
					pstruAV->u32AVFrameLen - idrOffset - 4, 
					true, 
					pstruAV->u32AVFramePTS);
				if (!bRet)
				{
					printf("Fail to EasyRTMP_SendH264Packet(I-frame) ...\n");
				}
				else
				{
					printf("I");
				}
			}
			else
			{
				if(rtmpHandle)
				{
					bRet = EasyRTMP_SendH264Packet(rtmpHandle,
						(unsigned char*)pu8Buffer+sizeof(HI_S_AVFrame)+4,
						pstruAV->u32AVFrameLen - 4, 
						false, 
						pstruAV->u32AVFramePTS);
					if (!bRet)
					{
						printf("Fail to EasyRTMP_SendH264Packet(P-frame) ...\n");
					}
					else
					{
						printf("P");
					}
				}
			}				



















		}
		else
		if (pstruAV->u32AVFrameFlag == HI_NET_DEV_AUDIO_FRAME_FLAG)
		{
			//printf("Audio %u PTS: %u \n", pstruAV->u32AVFrameLen, pstruAV->u32AVFramePTS);	
		}
	}
	else if (u32DataType == HI_NET_DEV_SYS_DATA)
	{
		pstruSys = (HI_S_SysHeader*)pu8Buffer;
		printf("Video W:%u H:%u Audio: %u \n", pstruSys->struVHeader.u32Width, pstruSys->struVHeader.u32Height, pstruSys->struAHeader.u32Format);
	} 

	return HI_SUCCESS;
}

HI_S32 OnDataCallback(HI_U32 u32Handle, /* 句柄 */
                                HI_U32 u32DataType,       /* 数据类型*/
                                HI_U8*  pu8Buffer,      /* 数据 */
                                HI_U32 u32Length,      /* 数据长度 */
                                HI_VOID* pUserData    /* 用户数据*/
                                )
{
	return HI_SUCCESS;
}

int main()
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S_STREAM_INFO struStreamInfo;
    HI_U32 a;
    
    HI_NET_DEV_Init();
    
    s32Ret = HI_NET_DEV_Login(&u32Handle, CNAME, CPWORD, CHOST, CPORT);
    if (s32Ret != HI_SUCCESS)
    {
        HI_NET_DEV_DeInit();
		return -1;
    }
    
	//HI_NET_DEV_SetEventCallBack(u32Handle, OnEventCallback, &a);
	HI_NET_DEV_SetStreamCallBack(u32Handle, (HI_ON_STREAM_CALLBACK)OnStreamCallback, &a);
	//HI_NET_DEV_SetDataCallBack(u32Handle, OnDataCallback, &a);

	struStreamInfo.u32Channel = HI_NET_DEV_CHANNEL_1;
	struStreamInfo.blFlag = HI_FALSE;//HI_FALSE;
	struStreamInfo.u32Mode = HI_NET_DEV_STREAM_MODE_TCP;
	struStreamInfo.u8Type = HI_NET_DEV_STREAM_ALL;
	s32Ret = HI_NET_DEV_StartStream(u32Handle, &struStreamInfo);
	if (s32Ret != HI_SUCCESS)
	{
		HI_NET_DEV_Logout(u32Handle);
		u32Handle = 0;
		return -1;
	}

	while(1)
	{
		Sleep(10);	
	};

	//是否RTMP推送
    EasyRTMP_Session_Release(rtmpHandle);
    rtmpHandle = 0;
   
    HI_NET_DEV_StopStream(u32Handle);
    HI_NET_DEV_Logout(u32Handle);
    
    HI_NET_DEV_DeInit();

    return 0;
}