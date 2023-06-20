/**
** 作者：pengweizhi
** 邮箱：1795387053@qq.com
**/

#include "ffcommon.h"
#include "AVOutputStream.h"

CAVOutputStream::CAVOutputStream(void)
{
	m_video_codec_id = AV_CODEC_ID_NONE;
	m_audio_codec_id = AV_CODEC_ID_NONE;
	m_width = 320;
	m_height = 240;
	m_framerate = 25;
	m_video_bitrate = 500000;
	m_samplerate = 0;
	m_channels = 1;
	m_audio_bitrate = 32000;
	video_st = NULL;
    audio_st = NULL;
	ofmt_ctx = NULL;
	pCodecCtx = NULL;
	pCodecCtx_a = NULL;
	pCodec = NULL;
	pCodec_a = NULL;
    m_fifo = NULL;
	pFrameYUV = NULL;
	img_convert_ctx = NULL;
	aud_convert_ctx = NULL;
	m_fifo = NULL;
	m_vid_framecnt = 0;
	m_nb_samples = 0;
	m_converted_input_samples = NULL;

	m_output_path = "";

	m_vid_framecnt = 0;
	m_aud_framecnt = 0;
    m_first_vid_time1 = m_first_vid_time2 = -1;
	m_first_aud_time = -1;
	m_next_vid_time = 0;
	m_next_aud_time = 0;

	m_nLastAudioPresentationTime = 0;
}

CAVOutputStream::~CAVOutputStream(void)
{
}



//初始化视频编码器
void CAVOutputStream::SetVideoCodecProp(AVCodecID codec_id, int framerate, int bitrate, int gopsize, int width, int height)
{
	 m_video_codec_id = codec_id;
	 m_width = width;
	 m_height = height;
	 m_framerate = ((framerate == 0) ? 10 : framerate);
	 m_video_bitrate = bitrate;
	 m_gopsize = gopsize;
}

//初始化音频编码器
void CAVOutputStream::SetAudioCodecProp(AVCodecID codec_id, int samplerate, int channels, int bitrate)
{
	m_audio_codec_id = codec_id;
	m_samplerate = samplerate;
	m_channels = channels;
	m_audio_bitrate = bitrate;
}


//创建编码器和混合器
bool CAVOutputStream::OpenOutputStream(const char* out_path)
{
	int ret = 0;
	m_output_path = out_path;

	//output initialize
	AVOutputFormat*  format = av_guess_format("rtp", NULL, NULL);
	avformat_alloc_output_context2(&ofmt_ctx, format, format->name, out_path);
	   
	if(m_video_codec_id != 0)
	{
		//output video encoder initialize
		pCodec = avcodec_find_encoder(m_video_codec_id);
		if (!pCodec)
		{
			printf("Can not find output video encoder! (没有找到合适的编码器！)\n");
			return false;
		}
		pCodecCtx = avcodec_alloc_context3(pCodec);
		pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
		pCodecCtx->width = m_width;
		pCodecCtx->height = m_height;
		pCodecCtx->time_base.num = 1;
		pCodecCtx->time_base.den = m_framerate;
		pCodecCtx->bit_rate = m_video_bitrate;
		pCodecCtx->gop_size = m_gopsize;
		/* Some formats want stream headers to be separate. */
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


		AVDictionary *param = 0;

		//set H264 codec param
		if(m_video_codec_id == AV_CODEC_ID_H264)
		{
			//pCodecCtx->me_range = 16;
			//pCodecCtx->max_qdiff = 4;
			//pCodecCtx->qcompress = 0.6;
			pCodecCtx->qmin = 10;
			pCodecCtx->qmax = 51;
			//Optional Param
			pCodecCtx->max_b_frames = 0;			

#if 1
			 //下面设置两个参数影响编码延时，如果不设置，编码器默认会缓冲很多帧
			// Set H264 preset and tune
			av_dict_set(&param, "preset", "fast", 0);
			av_dict_set(&param, "tune", "zerolatency", 0);
#else
			 /**
			  * ultrafast,superfast, veryfast, faster, fast, medium
			  * slow, slower, veryslow, placebo.　
			  注意：这是x264编码速度的选项， 设置该参数可以降低编码延时
			  */
			av_opt_set(pCodecCtx->priv_data,"preset","superfast",0);
#endif

		}

		if (avcodec_open2(pCodecCtx, pCodec, &param) < 0)
		{
			printf("Failed to open output video encoder! (编码器打开失败！)\n");
			return false;
		}

		//Add a new stream to output,should be called by the user before avformat_write_header() for muxing
		video_st = avformat_new_stream(ofmt_ctx, pCodec);
		if (video_st == NULL)
		{
			return false;
		}
		video_st->time_base.num = 1;
		video_st->time_base.den = m_framerate;

		/* copy the stream parameters to the muxer */
		ret = avcodec_parameters_from_context(video_st->codecpar, pCodecCtx);
		if (ret < 0) {
			fprintf(stderr, "Could not copy the stream parameters\n");
			exit(1);
		}

		//Initialize the buffer to store YUV frames to be encoded.
		pFrameYUV = av_frame_alloc();

		av_image_alloc(pFrameYUV->data, pFrameYUV->linesize, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, 1);	// TODO 需要释放
	}

	if(m_audio_codec_id != 0)
	{
		//output audio encoder initialize
		pCodec_a = avcodec_find_encoder(m_audio_codec_id);
		if (!pCodec_a)
		{
			printf("Can not find output audio encoder! (没有找到合适的编码器！)\n");
			return false;
		}
		pCodecCtx_a = avcodec_alloc_context3(pCodec_a);
		pCodecCtx_a->channels = m_channels;
		pCodecCtx_a->channel_layout = av_get_default_channel_layout(m_channels);
		pCodecCtx_a->sample_rate = m_samplerate;
		pCodecCtx_a->sample_fmt = pCodec_a->sample_fmts[0];
		pCodecCtx_a->bit_rate = m_audio_bitrate;
		pCodecCtx_a->time_base.num = 1;
		pCodecCtx_a->time_base.den = pCodecCtx_a->sample_rate;

		if(m_audio_codec_id == AV_CODEC_ID_AAC)
		{
			/** Allow the use of the experimental AAC encoder */
			pCodecCtx_a->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
		}

		/* Some formats want stream headers to be separate. */
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			pCodecCtx_a->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		if (avcodec_open2(pCodecCtx_a, pCodec_a, NULL) < 0)
		{
			printf("Failed to open ouput audio encoder! (编码器打开失败！)\n");
			return false;
		}

		//Add a new stream to output,should be called by the user before avformat_write_header() for muxing
		audio_st = avformat_new_stream(ofmt_ctx, pCodec_a);
		if (audio_st == NULL)
		{
			return false;
		}
		audio_st->time_base.num = 1;
		audio_st->time_base.den = pCodecCtx_a->sample_rate;

		/* copy the stream parameters to the muxer */
		ret = avcodec_parameters_from_context(audio_st->codecpar, pCodecCtx_a);
		if (ret < 0) {
			fprintf(stderr, "Could not copy the stream parameters\n");
			exit(1);
		}

		//Initialize the FIFO buffer to store audio samples to be encoded. 

		m_fifo = av_audio_fifo_alloc(pCodecCtx_a->sample_fmt, pCodecCtx_a->channels, 1);

		//Initialize the buffer to store converted samples to be encoded.
		m_converted_input_samples = NULL;
		/**
		* Allocate as many pointers as there are audio channels.
		* Each pointer will later point to the audio samples of the corresponding
		* channels (although it may be NULL for interleaved formats).
		* 这里申请的只是存储指针的十来个字节的内存。音频数据用的内存在 write_audio_frame()里用到时才申请
		*/
		if (!(m_converted_input_samples = (uint8_t**)calloc(pCodecCtx_a->channels, sizeof(**m_converted_input_samples)))) 
		{
			printf("Could not allocate converted input sample pointers\n");
			return false;
		}
		m_converted_input_samples[0] = NULL;
	}

	//Open output URL,set before avformat_write_header() for muxing
	if (avio_open(&ofmt_ctx->pb, ofmt_ctx->url, AVIO_FLAG_WRITE) < 0)
	{
		printf("Failed to open output file! (输出文件打开失败！)\n");
		return false;
	}

	//Show some Information
	av_dump_format(ofmt_ctx, 0, out_path, 1);

	//Write File Header
	ret = avformat_write_header(ofmt_ctx, NULL);

	m_vid_framecnt = 0;
	m_aud_framecnt = 0;
    m_nb_samples = 0;
    m_nLastAudioPresentationTime = 0;
    m_next_vid_time = 0;
	m_next_aud_time = 0;
    m_first_vid_time1 = m_first_vid_time2 = -1;
	m_first_aud_time = -1;

	/* Write a file for VLC */
	char buf[200000];
	AVFormatContext *ac[] = { ofmt_ctx };
	av_sdp_create(ac, 1, buf, 20000);
	printf("sdp:\n%s\n", buf);
	FILE* fsdp = fopen("test.sdp", "w");
	fprintf(fsdp, "%s", buf);
	fclose(fsdp);

	return true;
}

//input_st -- 输入流的信息
//input_frame -- 输入视频帧的信息
//lTimeStamp -- 时间戳，时间单位为1/1000000
//
int CAVOutputStream::write_video_frame(AVStream * input_st, enum AVPixelFormat pix_fmt, AVFrame *pframe, int64_t lTimeStamp)
{
	int ret = 0;

	if(video_st == NULL)
	   return -1;

	//printf("Video timestamp: %ld \n", lTimeStamp);

   if(m_first_vid_time1 == -1)
   {
	   printf("First Video timestamp: %lld \n", lTimeStamp);
	   m_first_vid_time1 = lTimeStamp;
   }

	AVRational time_base_q = { 1, AV_TIME_BASE };

	if(img_convert_ctx == NULL)
	{
		//camera data may has a pix fmt of RGB or sth else,convert it to YUV420
		img_convert_ctx = sws_getContext(m_width, m_height,
			pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	}

    sws_scale(img_convert_ctx, (const uint8_t* const*)pframe->data, pframe->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
    pFrameYUV->width = pframe->width;
    pFrameYUV->height = pframe->height;
    pFrameYUV->format = AV_PIX_FMT_YUV420P;

	// 编码并输出

	// send the frame to the encoder
	ret = avcodec_send_frame(pCodecCtx, pFrameYUV);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame to the encoder: %s\n",
			av_err2str(ret));
		exit(1);
	}

	while (ret >= 0) {
		AVPacket pkt = { 0 };

		ret = avcodec_receive_packet(pCodecCtx, &pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			fprintf(stderr, "Error encoding a frame: %s\n", av_err2str(ret));
			exit(1);
		}
			

		m_vid_framecnt++;

		printf("write video frame, package size:%d\n", pkt.size);			   		 	  

		/* rescale output packet timestamp values from codec to stream timebase */
		av_packet_rescale_ts(&pkt, pCodecCtx->time_base, video_st->time_base);
		pkt.stream_index = video_st->index;

		/* Write the compressed frame to the media file. */
		//log_packet(fmt_ctx, &pkt);
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		av_packet_unref(&pkt);
		if (ret < 0) {
			fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
			exit(1);
		}
	}


	return 0;
}

//input_st -- 输入流的信息
//input_frame -- 输入音频帧的信息
//lTimeStamp -- 时间戳，时间单位为1/1000000
//
int  CAVOutputStream::write_audio_frame(AVStream *input_st, AVFrame *input_frame, int64_t lTimeStamp)
{
	if(audio_st == NULL)
		return -1;

	if(m_first_aud_time == -1)
	{
		printf("First Audio timestamp: %lld \n", lTimeStamp);
		m_first_aud_time = lTimeStamp;
	}

	const int output_frame_size = pCodecCtx_a->frame_size;	// avcodec_open2(pCodecCtx_a, pCodec_a, NULL) 里，FFMPEG 设置了pCodecCtx_a->frame_size =1024

	AVRational time_base_q = { 1, AV_TIME_BASE };
	int ret;

	//if((int64_t)(av_audio_fifo_size(m_fifo) + input_frame->nb_samples) * AV_TIME_BASE /(int64_t)(input_st->codec->sample_rate) - lTimeStamp > AV_TIME_BASE/10)
	//{
	//	TRACE("audio data is overflow \n");
	//	return 0;
	//}

	int nFifoSamples = av_audio_fifo_size(m_fifo);
	int64_t timeshift = (int64_t)nFifoSamples * AV_TIME_BASE /(int64_t)(input_st->codecpar->sample_rate); //因为Fifo里有之前未读完的数据，所以从Fifo队列里面取出的第一个音频包的时间戳等于当前时间减掉缓冲部分的时长


	printf("audio time diff: %I64d \n", lTimeStamp - timeshift - m_nLastAudioPresentationTime); //理论上该差值稳定在一个水平，如果差值一直变大（在某些采集设备上发现有此现象），则会有视音频不同步的问题，具体产生的原因不清楚
    m_aud_framecnt += input_frame->nb_samples;


	if(aud_convert_ctx == NULL)
	{
		// Initialize the resampler to be able to convert audio sample formats
		aud_convert_ctx = swr_alloc_set_opts(NULL,
			av_get_default_channel_layout(pCodecCtx_a->channels),
			pCodecCtx_a->sample_fmt,
			pCodecCtx_a->sample_rate,
			av_get_default_channel_layout(input_st->codecpar->channels),
			(AVSampleFormat)input_st->codecpar->format,
			input_st->codecpar->sample_rate,
			0, NULL);

		/**
		* Perform a sanity check so that the number of converted samples is
		* not greater than the number of samples to be converted.
		* If the sample rates differ, this case has to be handled differently
		*/
		// TODO ATLASSERT(pCodecCtx_a->sample_rate == input_st->codec->sample_rate);

		swr_init(aud_convert_ctx);
	}

	/**
	* Allocate memory for the samples of all channels in one consecutive
	* block for convenience.
	* 用完后面要释放掉
	*/
	if ((ret = av_samples_alloc(m_converted_input_samples, NULL, pCodecCtx_a->channels, input_frame->nb_samples, pCodecCtx_a->sample_fmt, 0)) < 0)
	{
		printf("Could not allocate converted input samples\n");
		av_freep(&m_converted_input_samples[0]);
		return ret;
	}
	

	/**
	* Convert the input samples to the desired output sample format.
	* This requires a temporary storage provided by converted_input_samples.
	*/
	/** Convert the samples using the resampler. */
	if ((ret = swr_convert(aud_convert_ctx,
		m_converted_input_samples, input_frame->nb_samples,
		(const uint8_t**)input_frame->extended_data, input_frame->nb_samples)) < 0)
	{
		printf("Could not convert input samples\n");
		return ret;
	}

	/** Add the converted input samples to the FIFO buffer for later processing. */
	/**
	* Make the FIFO as large as it needs to be to hold both,
	* the old and the new samples.
	*/
	if ((ret = av_audio_fifo_realloc(m_fifo, av_audio_fifo_size(m_fifo) + input_frame->nb_samples)) < 0)
	{
		printf("Could not reallocate FIFO\n");
		return ret;
	}

	/** Store the new samples in the FIFO buffer. */
	if (av_audio_fifo_write(m_fifo, (void **)m_converted_input_samples, input_frame->nb_samples) < input_frame->nb_samples) 
	{
		printf("Could not write data to FIFO\n");
		return AVERROR_EXIT;
	}

	// 释放掉上面 av_samples_alloc 申请的内存
	av_freep(&m_converted_input_samples[0]);

	int64_t timeinc = (int64_t)pCodecCtx_a->frame_size * AV_TIME_BASE /(int64_t)(input_st->codecpar->sample_rate);
    
    //当前帧的时间戳不能小于上一帧的值 
	if(lTimeStamp - timeshift > m_nLastAudioPresentationTime )
	{
		m_nLastAudioPresentationTime = lTimeStamp - timeshift; 
	}
	
	int c = av_audio_fifo_size(m_fifo);
	while (av_audio_fifo_size(m_fifo) >= output_frame_size)
		/**
		* Take one frame worth of audio samples from the FIFO buffer,
		* encode it and write it to the output file.
		*/
	{
		/** Temporary storage of the output samples of the frame written to the file. */
		AVFrame *output_frame = av_frame_alloc();
		if (!output_frame)
		{
			ret = AVERROR(ENOMEM);
			return ret;
		}
		/**
		* Use the maximum number of possible samples per frame.
		* If there is less than the maximum possible frame size in the FIFO
		* buffer use this number. Otherwise, use the maximum possible frame size
		*/
		const int frame_size = FFMIN(av_audio_fifo_size(m_fifo), pCodecCtx_a->frame_size);


		/** Initialize temporary storage for one output frame. */
		/**
		* Set the frame's parameters, especially its size and format.
		* av_frame_get_buffer needs this to allocate memory for the
		* audio samples of the frame.
		* Default channel layouts based on the number of channels
		* are assumed for simplicity.
		*/
		/* 官方的example encode_audio.c：
			frame->nb_samples = c->frame_size; 
			frame->format = c->sample_fmt; 
			frame->channel_layout = c->channel_layout;
		*/
		output_frame->nb_samples = frame_size;
		output_frame->channel_layout = pCodecCtx_a->channel_layout;
		output_frame->format = pCodecCtx_a->sample_fmt;
		output_frame->sample_rate = pCodecCtx_a->sample_rate;

		/**
		* Allocate the samples of the created frame. This call will make
		* sure that the audio frame can hold as many samples as specified.
		*/
		if ((ret = av_frame_get_buffer(output_frame, 0)) < 0) 
		{
			printf("Could not allocate output frame samples\n");
			av_frame_free(&output_frame);
			return ret;
		}

		/**
		* Read as many samples from the FIFO buffer as required to fill the frame.
		* The samples are stored in the frame temporarily.
		*/
		if (av_audio_fifo_read(m_fifo, (void **)output_frame->data, frame_size) < frame_size) 
		{
			printf("Could not read data from FIFO\n");
			return AVERROR_EXIT;
		}

		/* send the frame for encoding */
		ret = avcodec_send_frame(pCodecCtx_a, output_frame);
		if (ret < 0) {
			fprintf(stderr, "Error sending the frame to the encoder\n");
			exit(1);
		}

		/** Encode one frame worth of audio samples. */
		/** Packet used for temporary storage. */
		AVPacket output_packet;
		av_init_packet(&output_packet);
		output_packet.data = NULL;
		output_packet.size = 0;

		/* read all the available output packets (in general there may be any number of them */
		while (ret >= 0) {
			ret = avcodec_receive_packet(pCodecCtx_a, &output_packet);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				fprintf(stderr, "Error encoding audio frame\n");
				exit(1);
			}

			//output_packet.flags |= AV_PKT_FLAG_KEY;
			output_packet.stream_index = audio_st->index;

			output_packet.pts = av_rescale_q(m_nLastAudioPresentationTime, time_base_q, audio_st->time_base);			

			printf("write audio frame, package size:%d\n", output_packet.size);

			/*
			av_interleaved_write_frame() 在写出数据之前必须将数据保存在内存中。交错是获取多个流(例如一个音频流，一个视频流)并以单调顺序序列化它们的过程。
			所以，如果你写了一个音频帧，它会保存在内存中，直到你写一个“稍后”出现的视频帧。
			一旦稍后的视频帧被写入，音频帧就可以被刷新'这样可以以不同的速度或在不同的线程中处理流，但输出仍然是单调的。
			如果您只写一个流(一个 acc 流，没有视频)，那么按照建议使用 av_write_frame()。
			*/
			//if ((ret = av_interleaved_write_frame(ofmt_ctx, &output_packet)) < 0) 
			if ((ret = av_write_frame(ofmt_ctx, &output_packet)) < 0)
			{
				char tmpErrString[128] = { 0 };
				printf("Could not write audio frame, error: %s\n", av_make_error_string(tmpErrString, AV_ERROR_MAX_STRING_SIZE, ret));
				av_packet_unref(&output_packet);
				return ret;
			}

			av_packet_unref(&output_packet);
		}//if (enc_got_frame_a)
			   		 
		m_nb_samples += output_frame->nb_samples;

		m_nLastAudioPresentationTime += timeinc;

		av_frame_free(&output_frame);		
	}//while


	return 0;
}

void  CAVOutputStream::CloseOutput()
{
	if(ofmt_ctx != NULL)
	{
		if(video_st != NULL || audio_st != NULL)
		{
		 //Write file trailer
		  av_write_trailer(ofmt_ctx);
		}
	}

    if (pCodecCtx)
		avcodec_free_context(&pCodecCtx);
    if (pCodecCtx_a)
		avcodec_free_context(&pCodecCtx_a);

	// TODO 对应 OpenOutputStream 里的 av_image_alloc 
    if(pFrameYUV && pFrameYUV->data)
	{
		av_free(pFrameYUV->data);
	}

	if (m_converted_input_samples) 
	{
		av_freep(&m_converted_input_samples[0]);
		free(m_converted_input_samples);
		m_converted_input_samples = NULL;
	}

	if (m_fifo)
	{
		av_audio_fifo_free(m_fifo);
		m_fifo = NULL;
	}

	if (aud_convert_ctx)
	{
		swr_free(&aud_convert_ctx);
		aud_convert_ctx = NULL;
	}

	if(ofmt_ctx)
        avio_close(ofmt_ctx->pb);

    avformat_free_context(ofmt_ctx);

    m_video_codec_id = AV_CODEC_ID_NONE;
	m_audio_codec_id = AV_CODEC_ID_NONE;

	ofmt_ctx = NULL;
	video_st = NULL;
    audio_st = NULL;
}