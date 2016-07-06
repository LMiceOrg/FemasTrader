/********************************************************
	读取配置文件类
	文件名: profile.h
	说明: 
	创建: 
	修改: 吴舸 2001-04-20
	修改原因: 规范编码
**********************************************************/
#ifndef __PROFILE_H
#define __PROFILE_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <memory.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <float.h>
char *mytrim(char *s);

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif



// GetLocalTime只精确到毫秒，使用新的WindowsAPI，获取Windows平台的微秒时间
// 输入：v，一个long或LONGLONG的引用
// 输出：12位十进制整数，低6位为微秒时间，高6位为自机器开机以来的秒数
// 经测试，每次调用此方法，开销在1~2微秒之间
// -- lidp 20141216
#ifdef WIN32
#define GET_ACCURATE_USEC_TIME(v) 			\
	{                           \
        LARGE_INTEGER tick;     \
        LARGE_INTEGER timestamp;\
        QueryPerformanceFrequency(&tick);   \
        QueryPerformanceCounter(&timestamp);    \
        LONGLONG us = (timestamp.QuadPart % tick.QuadPart) * 1000000 / tick.QuadPart;   \
        LONGLONG seconds = timestamp.QuadPart / tick.QuadPart;  \
        v = (seconds % 1000000) * 1000000 + us; \
	}
#else
#define GET_ACCURATE_USEC_TIME(v)			\
	{					        \
		struct timeval t;		\
		gettimeofday(&t,NULL);		\
		tm *now=localtime(&t.tv_sec);		\
		v=(long)now->tm_hour*3600000000 + (long)now->tm_min*60000000+now->tm_sec*1000000+		\
			t.tv_usec;		\
   	}
#endif


#ifdef WIN32
#define GET_SEC_USEC_TIME(sec, usec) 			\
	{                           \
        LARGE_INTEGER tick;     \
        LARGE_INTEGER timestamp;\
        QueryPerformanceFrequency(&tick);   \
        QueryPerformanceCounter(&timestamp);    \
        usec = (timestamp.QuadPart % tick.QuadPart) * 1000000 / tick.QuadPart;   \
        sec = timestamp.QuadPart / tick.QuadPart;  \
	}
#else
#define GET_SEC_USEC_TIME(sec, usec) 			\
	{					\
		struct timeval t;		\
		gettimeofday(&t,NULL);		\
		tm *now=localtime(&t.tv_sec);		\
        sec = t.tv_sec; \
        usec = t.tv_usec; \
   	}
#endif



///Sleep 使用的参数不一样，这里说明了Sleep是以秒为级别的
#ifdef WIN32
#define SLEEP_SECONDS(seconds) Sleep((seconds)*1000)
#else
#define SLEEP_SECONDS(seconds) sleep(seconds)
#endif

class TIniFile
{
	//输入输出流
   FILE *fin,*fout;
	//返回指向section名称的字符串指针
	char *titlePos( char *buf, int *len );
	//是否是一个section字符串
	bool isTitleLine( char *bufPtr );
	//是否是指定的section
	bool containTitle( char *buf, const char *section );
	//移动文件指针到相应的section位置
	bool gotoSection(const char *section );
	//返回指向entry名称的字符串指针
	char *textPos( char *buf, const char *entry );
	//去除字符串前面的空格
	void stripQuotationChar( char *buf );
	//读取一个entry的值
	int readEntry( const char *entry, char *buf, int bufSize, bool strip );
	//写一个entry


public:
   // 函数名: TIniFile::ReadTString
   // 编程  : 陈永华 2004-5-7 20:40:18
   // 描述  : 新增的读取定义字段的字符串，且过滤左右空格和'\t'
   // 返回  : char * 
   // 参数  : const char *section
   // 参数  : const char *entry
   // 参数  : const char *defaultString
   // 参数  : char *buffer
   // 参数  : int bufLen
   char * ReadTString(const char *section,const char *entry,const char *defaultString,char *buffer,int   bufLen);
   // 函数名: TIniFile::LRTrim
   // 编程  : 陈永华 2004-5-7 20:41:24
   // 描述  : 从str中将左右空格和'\t'截取掉
   // 返回  : char * 
   // 参数  : char *str
   static char * LRTrim(char *str);
	//配置文件名称
	char FileName[128];
	//打开配置文件
	bool Open(char *filename);
	//读取一个int型entry的值
	int ReadInt( const char *section, const char *entry, int defaultInt);
	//读取一个string型的entry的值
	int ReadString( const char *section, const char *entry,const char *defaultString, char *buffer,int bufLen);
	//写一个string型的entry到指定的section中
	//写一个int型的entry到指定的section中
	//关闭配置文件
	void Close();

   TIniFile();
	~TIniFile();
private:
	char * FGetS(char *pbuf,int size,FILE *fp);
	void out_close();
	void in_close();
	char * percolate(char *str);
};

#ifdef WIN32
#define PATH_SPLIT '\\'
#else
#define PATH_SPLIT '/'
#endif

#define ALL_SPLITS "\\/$"

#define MAX_PATH_LEN 200
	
///转换路径
///@param	target	目标
///@param	source	源
void convertPath(char *target, const char *source);
	
///替换标准的fopen函数
FILE *mfopen(const char *filename, const char *mode);
#endif
