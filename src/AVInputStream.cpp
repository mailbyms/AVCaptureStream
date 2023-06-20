
/**
** 作者：pengweizhi
** 邮箱：1795387053@qq.com
**/

#include "windows.h"
#include "AVInputStream.h"
#include "ffcommon.h"

#define ATLTRACE printf

static std::string AnsiToUTF8(const char *_ansi, int _ansi_len)
{
	std::string str_utf8("");
	wchar_t* pUnicode = NULL;
	BYTE * pUtfData = NULL;
	do
	{
		int unicodeNeed = MultiByteToWideChar(CP_ACP, 0, _ansi, _ansi_len, NULL, 0);
		pUnicode = new wchar_t[unicodeNeed + 1];
		memset(pUnicode, 0, (unicodeNeed + 1)*sizeof(wchar_t));
		int unicodeDone = MultiByteToWideChar(CP_ACP, 0, _ansi, _ansi_len, (LPWSTR)pUnicode, unicodeNeed);

		if (unicodeDone != unicodeNeed)
		{
			break;
		}

		int utfNeed = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)pUnicode, unicodeDone, (char *)pUtfData, 0, NULL, NULL);
		pUtfData = new BYTE[utfNeed + 1];
		memset(pUtfData, 0, utfNeed + 1);
		int utfDone = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)pUnicode, unicodeDone, (char *)pUtfData, utfNeed, NULL, NULL);

		if (utfNeed != utfDone)
		{
			break;
		}
		str_utf8.assign((char *)pUtfData);
	} while (false);

	if (pUnicode)
	{
		delete[] pUnicode;
	}
	if (pUtfData)
	{
		delete[] pUtfData;
	}

	return str_utf8;
}

CAVInputStream::CAVInputStream(void)
{
	m_hCapVideoThread = NULL;
	m_hCapAudioThread = NULL;
	m_exit_thread = false;

	m_pVidFmtCtx = NULL;
	m_pAudFmtCtx = NULL;
	m_pInputFormat = NULL;
	
	m_pVideoCBFunc = NULL;
	m_pAudioCBFunc = NULL;

	m_videoindex = -1;
	m_audioindex = -1;

	m_start_time = 0;

	//avcodec_register_all();
 //   av_register_all();
	//avdevice_register_all();  
}

CAVInputStream::~CAVInputStream(void)
{
	CloseInputStream();
}

	
void  CAVInputStream::SetVideoCaptureCB(VideoCaptureCB pFuncCB)
{
	m_pVideoCBFunc = pFuncCB;
}
	
void  CAVInputStream::SetAudioCaptureCB(AudioCaptureCB pFuncCB)
{
	m_pAudioCBFunc = pFuncCB;
}
	
void  CAVInputStream::SetVideoCaptureDevice(string device_name)
{
	m_video_device = device_name;
}

void  CAVInputStream::SetAudioCaptureDevice(string device_name)
{
	m_audio_device = device_name;
}


bool  CAVInputStream::OpenInputStream()
{
	if(m_video_device.empty() && m_audio_device.empty())
	{
		ATLTRACE("you have not set any capture device \n");
		return false;
	}


	int i;
	enum AVMediaType type;

	//打开Directshow设备前需要调用FFmpeg的avdevice_register_all函数，否则下面返回失败
	m_pInputFormat = av_find_input_format("dshow");
	// TODO ASSERT(m_pInputFormat != NULL);

    // Set device params
    AVDictionary *device_param = 0;
	//if not setting rtbufsize, error messages will be shown in cmd, but you can still watch or record the stream correctly in most time
	//setting rtbufsize will erase those error messages, however, larger rtbufsize will bring latency
    //av_dict_set(&device_param, "rtbufsize", "10M", 0);

	if(!m_video_device.empty())
	{
		int res = 0;
		type = AVMEDIA_TYPE_VIDEO;

		string device_name = "video=" + m_video_device;

		string device_name_utf8 = AnsiToUTF8(device_name.c_str(), device_name.length());  //转成UTF-8，解决设备名称包含中文字符出现乱码的问题

		 //Set own video device's name
		if ((res = avformat_open_input(&m_pVidFmtCtx, device_name_utf8.c_str(), m_pInputFormat, &device_param)) != 0)
		{
			ATLTRACE("Couldn't open input video stream.（无法打开输入流）\n");
			return false;
		}
		//input video initialize
		if (avformat_find_stream_info(m_pVidFmtCtx, NULL) < 0)
		{
			ATLTRACE("Couldn't find video stream information.（无法获取流信息）\n");
			return false;
		}

		int ret = av_find_best_stream(m_pVidFmtCtx, type, -1, -1, NULL, 0);
		if (ret < 0) {
			fprintf(stderr, "Could not find %s stream\n",
				av_get_media_type_string(type));
			return ret;
		}
		else {
			m_videoindex = ret;

		}

		AVStream *st = m_pVidFmtCtx->streams[m_videoindex];

		/* find decoder for the stream */
		AVCodec *dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec) {
			fprintf(stderr, "Failed to find %s codec\n",
				av_get_media_type_string(type));
			return false;
		}

		/* Allocate a codec context for the decoder */
		m_video_dec_ctx = avcodec_alloc_context3(dec);
		if (!m_video_dec_ctx) {
			fprintf(stderr, "Failed to allocate the %s codec context\n",
				av_get_media_type_string(type));
			return false;
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(m_video_dec_ctx, st->codecpar)) < 0) {
			fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
				av_get_media_type_string(type));
			return false;
		}

		/* Init the decoders */
		if ((ret = avcodec_open2(m_video_dec_ctx, dec, NULL)) < 0) {
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return false;
		}
	}

    //////////////////////////////////////////////////////////

	if(!m_audio_device.empty())
	{
		type = AVMEDIA_TYPE_AUDIO;

		string device_name = "audio=" + m_audio_device;

		string device_name_utf8 = AnsiToUTF8(device_name.c_str(), device_name.length());  //转成UTF-8，解决设备名称包含中文字符出现乱码的问题

		//Set own audio device's name
		if (avformat_open_input(&m_pAudFmtCtx, device_name_utf8.c_str(), m_pInputFormat, &device_param) != 0){

			ATLTRACE("Couldn't open input audio stream.（无法打开输入流）\n");
			return false;
		}

		//input audio initialize
		if (avformat_find_stream_info(m_pAudFmtCtx, NULL) < 0)
		{
			ATLTRACE("Couldn't find audio stream information.（无法获取流信息）\n");
			return false;
		}
		

		int ret = av_find_best_stream(m_pAudFmtCtx, type, -1, -1, NULL, 0);
		if (ret < 0) {
			fprintf(stderr, "Could not find %s stream\n",
				av_get_media_type_string(type));
			return ret;
		}
		else {
			m_audioindex = ret;

		}

		AVStream *st = m_pAudFmtCtx->streams[m_audioindex];

		/* find decoder for the stream */
		AVCodec *dec = avcodec_find_decoder(st->codecpar->codec_id);
		if (!dec) {
			fprintf(stderr, "Failed to find %s codec\n",
				av_get_media_type_string(type));
			return false;
		}

		/* Allocate a codec context for the decoder */
		m_audio_dec_ctx = avcodec_alloc_context3(dec);
		if (!m_audio_dec_ctx) {
			fprintf(stderr, "Failed to allocate the %s codec context\n",
				av_get_media_type_string(type));
			return false;
		}

		/* Copy codec parameters from input stream to output codec context */
		if ((ret = avcodec_parameters_to_context(m_audio_dec_ctx, st->codecpar)) < 0) {
			fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
				av_get_media_type_string(type));
			return false;
		}

		/* Init the decoders */
		if ((ret = avcodec_open2(m_audio_dec_ctx, dec, NULL)) < 0) {
			fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
			return false;
		}
	}

	return true;
}

bool  CAVInputStream::StartCapture()
{
	if (m_videoindex == -1 && m_audioindex == -1)
	{
		ATLTRACE("错误：你没有打开设备 \n");
		return false;
	}

    m_start_time = av_gettime();

	m_exit_thread = false;

	if(!m_video_device.empty())
	{
		m_hCapVideoThread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        CaptureVideoThreadFunc,       // thread function name
        this,          // argument to thread function 
        0,                      // use default creation flags 
        NULL);   // returns the thread identifier 
	}

	if(!m_audio_device.empty())
	{
		m_hCapAudioThread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        CaptureAudioThreadFunc,       // thread function name
        this,          // argument to thread function 
        0,                      // use default creation flags 
        NULL);   // returns the thread identifier 
	}

	return true;
}

void  CAVInputStream::CloseInputStream()
{
	m_exit_thread = true;

	if(m_hCapVideoThread)
	{  
		if( WAIT_TIMEOUT == WaitForSingleObject(m_hCapVideoThread, 3000) )
		{
			printf("WaitForSingleObject timeout.\n");
			exit(-1);
			//::TerminateThread(m_hCapVideoThread, 0);
		}
		CloseHandle(m_hCapVideoThread);
		m_hCapVideoThread = NULL;
	}

	if(m_hCapAudioThread)
	{  
		if( WAIT_TIMEOUT == WaitForSingleObject(m_hCapAudioThread, 3000) )
		{
			printf("WaitForSingleObject timeout.\n");
			exit(-1);
			//::TerminateThread(m_hCapAudioThread, 0);
		}
		CloseHandle(m_hCapAudioThread);
		m_hCapAudioThread = NULL;
	}

	 //关闭输入流  
    if (m_pVidFmtCtx != NULL)  
    {  
        avformat_close_input(&m_pVidFmtCtx);  
        //m_pVidFmtCtx = NULL;  
    }  
    if (m_pAudFmtCtx != NULL)  
    {  
        avformat_close_input(&m_pAudFmtCtx);  
        //m_pAudFmtCtx = NULL;  
    }  

	if(m_pVidFmtCtx)
	   avformat_free_context(m_pVidFmtCtx);
	if(m_pAudFmtCtx)
	   avformat_free_context(m_pAudFmtCtx);

    m_pVidFmtCtx = NULL;
    m_pAudFmtCtx = NULL;
    m_pInputFormat = NULL;

	m_videoindex = -1;
	m_audioindex = -1;
}

DWORD WINAPI CAVInputStream::CaptureVideoThreadFunc(LPVOID lParam)
{
	CAVInputStream * pThis = (CAVInputStream*)lParam;

	pThis->ReadVideoPackets();

	return 0;
}

int CAVInputStream::decode_packet(AVCodecContext *dec, const AVPacket *pkt)
{
	int ret = 0;
	AVFrame *frame = NULL;

	// submit the packet to the decoder
	ret = avcodec_send_packet(dec, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
		return ret;
	}

	// get all the available frames from the decoder
	while (ret >= 0) {
		frame = av_frame_alloc();
		if (!frame)
		{
			ret = AVERROR(ENOMEM);
			return ret;
		}

		ret = avcodec_receive_frame(dec, frame);
		if (ret < 0) {
			// those two return values are special and mean there is no output
			// frame available, but there were no errors during decoding
			if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
				return 0;

			fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
			return ret;
		}

		// write the frame data to output file
		if (dec->codec->type == AVMEDIA_TYPE_VIDEO) {
			if (m_pVideoCBFunc)
			{
				CAutoLock lock(&m_WriteLock);

				m_pVideoCBFunc(m_pVidFmtCtx->streams[m_videoindex], m_pVidFmtCtx->streams[m_videoindex]->codec->pix_fmt, frame, av_gettime() - m_start_time);
			}

			av_frame_free(&frame);
		}
		else if (dec->codec->type == AVMEDIA_TYPE_AUDIO) {
			if (m_pAudioCBFunc)
			{
				CAutoLock lock(&m_WriteLock);

				m_pAudioCBFunc(m_pAudFmtCtx->streams[m_audioindex], frame, av_gettime() - m_start_time);
			}

			av_frame_free(&frame);
		}

		av_frame_unref(frame);
		if (ret < 0)
			return ret;
	}

	return 0;
}

int  CAVInputStream::ReadVideoPackets()
{
	////prepare before decode and encode
	AVPacket *dec_pkt = (AVPacket *)av_malloc(sizeof(AVPacket));

	int encode_video = 1;
	int ret;

	//start decode and encode

	while (encode_video)
	{
		if (m_exit_thread)
			break;

		AVFrame * pframe = NULL;
		if ((ret = av_read_frame(m_pVidFmtCtx, dec_pkt)) >= 0)
		{
			ret = decode_packet(m_video_dec_ctx, dec_pkt);

			av_packet_unref(dec_pkt);
			if (ret < 0)
				break;
		}
		else
		{
			if (ret == AVERROR_EOF)
				encode_video = 0;
			else
			{
				ATLTRACE("Could not read video frame\n");
				break;
			}
		}
	}

	return 0;
}

DWORD WINAPI CAVInputStream::CaptureAudioThreadFunc(LPVOID lParam)
{
	CAVInputStream * pThis = (CAVInputStream*)lParam;

	pThis->ReadAudioPackets();

	return 0;
}

int CAVInputStream::ReadAudioPackets()
{
	//audio trancoding here
    int ret;

	int encode_audio = 1;
    int dec_got_frame_a = 0;

	//start decode and encode
	while (encode_audio)
	{
		if (m_exit_thread)
		   break;

		/**
		* Decode one frame worth of audio samples, convert it to the
		* output sample format and put it into the FIFO buffer.
		*/
		AVFrame *input_frame = av_frame_alloc();
		if (!input_frame)
		{
			ret = AVERROR(ENOMEM);
			return ret;
		}			

		/** Decode one frame worth of audio samples. */
		/** Packet used for temporary storage. */
		AVPacket input_packet;
		av_init_packet(&input_packet);
		input_packet.data = NULL;
		input_packet.size = 0;

		/** Read one audio frame from the input file into a temporary packet. */
		if ((ret = av_read_frame(m_pAudFmtCtx, &input_packet)) < 0) 
		{
			/** If we are at the end of the file, flush the decoder below. */
			if (ret == AVERROR_EOF)
			{
				encode_audio = 0;
			}
			else
			{
				ATLTRACE("Could not read audio frame\n");
				return ret;
			}					
		}

		ret = decode_packet(m_audio_dec_ctx, &input_packet);

		av_packet_unref(&input_packet);
		if (ret < 0)
			break;
			
		av_packet_unref(&input_packet);
				

	}//while

	return 0;
}

	
bool CAVInputStream::GetVideoInputInfo(int & width, int & height, int & frame_rate, AVPixelFormat & pixFmt)
{
	if(m_videoindex != -1)
	{
		width  =  m_pVidFmtCtx->streams[m_videoindex]->codec->width;
		height =  m_pVidFmtCtx->streams[m_videoindex]->codec->height;
		
		AVStream *stream = m_pVidFmtCtx->streams[m_videoindex];  
  
		pixFmt = stream->codec->pix_fmt;

        //frame_rate = stream->avg_frame_rate.num/stream->avg_frame_rate.den;//每秒多少帧

		if(stream->r_frame_rate.den > 0)
		{
		  frame_rate = stream->r_frame_rate.num/stream->r_frame_rate.den;
		}
		else if(stream->codec->framerate.den > 0)
		{
		  frame_rate = stream->codec->framerate.num/stream->codec->framerate.den;
		}

		return true;
	}
	return false;
}

bool  CAVInputStream::GetAudioInputInfo(AVSampleFormat & sample_fmt, int & sample_rate, int & channels)
{
	if(m_audioindex != -1)
	{
		sample_fmt = m_pAudFmtCtx->streams[m_audioindex]->codec->sample_fmt;
		sample_rate = m_pAudFmtCtx->streams[m_audioindex]->codec->sample_rate;
		channels = m_pAudFmtCtx->streams[m_audioindex]->codec->channels;

		return true;
	}
	return false;
}