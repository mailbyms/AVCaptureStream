#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <windows.h>
#include <mmsystem.h>

#include "AVInputStream.h"
#include "AVOutputStream.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")

#define DEVICE_NUM 1 // 请设置正确的设备编号

char              m_szFilePath[256];

CAVInputStream    m_InputStream;
CAVOutputStream   m_OutputStream;

//采集到的视频图像回调
LRESULT CALLBACK VideoCaptureCallback(AVStream * input_st, enum AVPixelFormat pix_fmt, AVFrame *pframe, INT64 lTimeStamp)
{
	m_OutputStream.write_video_frame(input_st, pix_fmt, pframe, lTimeStamp);
	return 0;
}

//采集到的音频数据回调
LRESULT CALLBACK AudioCaptureCallback(AVStream * input_st, AVFrame *pframe, INT64 lTimeStamp)
{
	m_OutputStream.write_audio_frame(input_st, pframe, lTimeStamp);
	return 0;
}

LRESULT  OnStartStream(WPARAM wParam, LPARAM lParam)
{
	m_InputStream.SetVideoCaptureCB(VideoCaptureCallback);
	m_InputStream.SetAudioCaptureCB(AudioCaptureCallback);

	bool bRet;
	bRet = m_InputStream.OpenInputStream(); //初始化采集设备
	if (!bRet)
	{
		printf("打开采集设备失败\n");
		return 1;
	}

	int cx, cy, fps;
	AVPixelFormat pixel_fmt;
	if (m_InputStream.GetVideoInputInfo(cx, cy, fps, pixel_fmt)) //获取视频采集源的信息
	{
		m_OutputStream.SetVideoCodecProp(AV_CODEC_ID_H264, fps, 500000, 100, cx, cy); //设置视频编码器属性
	}

	int sample_rate = 0, channels = 0;
	AVSampleFormat  sample_fmt;
	if (m_InputStream.GetAudioInputInfo(sample_fmt, sample_rate, channels)) //获取音频采集源的信息
	{
		m_OutputStream.SetAudioCodecProp(AV_CODEC_ID_AAC, sample_rate, channels, 32000); //设置音频编码器属性
	}

	//从Config.INI文件中读取录制文件路径
	// P_GetProfileString(_T("Client"), "file_path", m_szFilePath, sizeof(m_szFilePath));
	strcpy(m_szFilePath, "rtp://127.0.0.1:49990");

	bRet = m_OutputStream.OpenOutputStream(m_szFilePath); //设置输出路径
	if (!bRet)
	{
		printf("初始化输出失败\n");
		return 1;
	}

	bRet = m_InputStream.StartCapture(); //开始采集

	return 0;
}

int main(int argc, char *argv[]) {
	// 初始化FFMPEG库
	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	//m_InputStream.SetVideoCaptureDevice("Logi C270 HD WebCam");
	m_InputStream.SetAudioCaptureDevice("麦克风 (Logi C270 HD WebCam)");
	OnStartStream(0, 0);

	while (true)
	{
		Sleep(100);
	}

	return 0;
}