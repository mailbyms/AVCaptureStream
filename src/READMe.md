## 一、项目来源
- 原始项目代码来自 csdn 下载：https://blog.csdn.net/zhoubotong2012/article/details/79338093    
- 修改项：
  - 去掉项目里调用 EnumDevice.dll 列举系统的设备的逻辑，改用 github 另一个开源的类：https://github.com/mailbyms/OpenCVDeviceEnumerator
  - MainFrm.cpp 里从配置文件读取输出文件名，改为固定值 capture.mkv


## 二、项目建立
Visual Studio 2017， Windows 10，Thinkpad X13 自带摄像头/话筒，台式机+罗技c270 USB 摄像头， 正常运行
- IDE 设置	
	- 项目属性，切换到 Debug - Win32。注意项目自带的 ffmpeg 库是 32 位的，版本为 December 5, 2014, FFmpeg 2.5 https://ffmpeg.org/index.html#news
	  ```
	  链接器
	      常规：输出文件，恢复为IDE默认的目录
		  高级：映像具有安全异常处理程序，改为否
		  调试: 环境，改为 PATH=%PATH%;..\bin （自带的 ffmpeg dll 库在 ..\bin）
	  ```

