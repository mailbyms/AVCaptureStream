
#ifndef FF_COMMON_H
#define FF_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif 

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/fifo.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/error.h"
#include "libswresample/swresample.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/time.h"
#include "libavdevice/avdevice.h"
#ifdef __cplusplus
}
#endif

#pragma comment( lib, "avcodec.lib")
#pragma comment( lib, "avutil.lib")
#pragma comment( lib, "avformat.lib")
#pragma comment(lib,   "swresample.lib")
#pragma comment(lib,  "swscale.lib" )
#pragma comment(lib, "avdevice.lib")


#endif
