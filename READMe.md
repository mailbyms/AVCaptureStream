## һ����Ŀ��Դ
- ԭʼ��Ŀ�������� csdn ���أ�https://blog.csdn.net/zhoubotong2012/article/details/79338093    
- �޸��
  - ȥ����Ŀ����� EnumDevice.dll �о�ϵͳ���豸���߼������� github ��һ����Դ���ࣺhttps://github.com/mailbyms/OpenCVDeviceEnumerator
  - MainFrm.cpp ��������ļ���ȡ����ļ�������Ϊ�̶�ֵ capture.mkv


## ������Ŀ����
Visual Studio 2017�� Windows 10��Thinkpad X13 �Դ�����ͷ/��Ͳ��̨ʽ��+�޼�c270 USB ����ͷ�� ��������
- ��Ŀԭ���Դ��� ffmpeg ���� 32 λ�ģ��汾Ϊ December 5, 2014, FFmpeg 2.5 https://ffmpeg.org/index.html#news ���ָ�Ϊ 64 λ FFmpeg 4.3.2��������к������ɵľ��棬�ɺ���
- ���� FFMPEG 64λ SDK (https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-4.3.2-full_build-shared.7z)������ѹ�� C �̸�Ŀ¼��

- IDE ��� FFMPEG SDK:
	
	- ��Ŀ���ԣ��л��� Debug - x64
	  ```
	  C/C++
	      ����->����Ŀ¼������ ffmpeg �� include Ŀ¼������"C:\ffmpeg-4.3.2\include"
	  ������
	      ����->���ӿ�Ŀ¼������ ffmpeg �� lib Ŀ¼������"C:\ffmpeg-4.3.2\lib"
	  ����
	      ������PATH=%PATH%;C:\ffmpeg-4.3.2\bin
	  ```

## ����ע������
����Դ�� main() �������ã�ͬʱֻ������Ƶ����Ƶ���Դ�����˵����Ƶ����Ƶ�ǵ������豸
