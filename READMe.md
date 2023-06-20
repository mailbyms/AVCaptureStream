## 一、项目来源
- 原始项目代码来自 csdn 下载：https://blog.csdn.net/zhoubotong2012/article/details/79338093    
- 修改项：
  - 去掉项目里调用 EnumDevice.dll 列举系统的设备的逻辑，改用 github 另一个开源的类：https://github.com/mailbyms/OpenCVDeviceEnumerator
  - MainFrm.cpp 里从配置文件读取输出文件名，改为固定值 capture.mkv


## 二、项目建立
Visual Studio 2017， Windows 10，Thinkpad X13 自带摄像头/话筒，台式机+罗技c270 USB 摄像头， 正常运行
- 项目原来自带的 ffmpeg 库是 32 位的，版本为 December 5, 2014, FFmpeg 2.5 https://ffmpeg.org/index.html#news 。现改为 64 位 FFmpeg 4.3.2，编译会有函数过旧的警告，可忽略
- 下载 FFMPEG 64位 SDK (https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-4.3.2-full_build-shared.7z)，并解压到 C 盘根目录下

- IDE 添加 FFMPEG SDK:
	
	- 项目属性，切换到 Debug - x64
	  ```
	  C/C++
	      常规->附加目录：增加 ffmpeg 的 include 目录，例如"C:\ffmpeg-4.3.2\include"
	  链接器
	      常规->附加库目录：增加 ffmpeg 的 lib 目录，例如"C:\ffmpeg-4.3.2\lib"
	  调试
	      环境：PATH=%PATH%;C:\ffmpeg-4.3.2\bin
	  ```

## 三、注意事项
输入源在 main() 函数设置，同时只能用视频或音频。对代码来说，视频和音频是单独的设备
