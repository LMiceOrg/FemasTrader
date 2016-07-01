/********************************************************
	读取配置文件类实现
	文件名: profile.cpp
	说明: 
	创建: 
	修改: 吴舸 2001-04-20
	修改原因: 规范编码
**********************************************************/
#include "profile.h"

/* 去除左右空格   */
char *mytrim(char *s)
{
	int i,len;

	len=strlen(s);
	for (i=len-1;i>=0;i--)
	{
		if ((s[i] != ' ') && (s[i] != '\t'))
			break;
		else
			s[i]=0;
	}
	for (i=0; i<len; i++)
	{
		if ((s[i] != ' ') && (s[i] != '\t'))
			break;
	}
	if (i != 0)
	{
		strncpy(s,s+i,len-i);
		s[len-i]=0;
	}
	return s;
}


TIniFile::TIniFile()
{
/*
	功能：关闭输入输出流
	参数：无
	返回值：无
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
   fin = NULL;
   fout = NULL;   
}

TIniFile::~TIniFile()
{
/*
	功能：关闭输入输出流
	参数：无
	返回值：无
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
   Close();   
}
void TIniFile::Close()
{
/*
	功能：关闭输入输出流
	参数：无
	返回值：无
	修改者：吴舸 2001-06-07
	修改原因：规范编码
*/
   in_close();
   out_close();
}

bool TIniFile::Open(char *filename)
{
/*
	功能：打开配置文件
	参数：
		filename[IN]：配置文件名
	返回值：
		成功返回true
		失败返回false
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
    //char tmpfilename[256];
    in_close();

/* ****** Updated by CHENYH at 2004-3-30 8:48:04 ****** 
    if (filename[0] != '.') sprintf(tmpfilename,"./%s",filename);
    if (access(filename,0))
    {
		printf("%s file does not exist in current directory!\n",filename);
		return false;
    }
    strcpy(FileName,tmpfilename);
    fin = fopen(FileName,"r");
    return true;
*/
    fin=fopen(filename,"rt");
    return(fin!=NULL);
}
char *TIniFile::titlePos( char *buf, int *len )
{
/*
	功能：取得指向section的开始的指针，及在buf中的偏移量
	参数：
		buf[IN]：源串
		len[OUT]：偏移量
	返回值：
		是一个section则返回指向该section起始的指针
		否则返回0
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
	char *p = buf, *q;

	while( *p && isspace(*p) ) p++;
	if( *p != '[' )
		return 0;

	q = p+1;
	while( *q && *q != ']' ) q++;
	if( *q != ']' )
		return 0;
	if( len )
		*len = int(q - p - 1);
	return p+1;
}

bool TIniFile::isTitleLine( char *bufPtr )
{
/*
	功能：字符串中是否是一个section
	参数：
		bufPtr[IN]：源串
	返回值：
		成功返回true
		失败返回false
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
	return titlePos( bufPtr, 0 ) != 0;
}

bool TIniFile::containTitle( char *buf, const char *section )
{
/*
	功能：字符串中是否包含指定的section
	参数：
		buf[IN]：源串
		section[IN]：要查找的section
	返回值：
		找到返回true
		否则返回false
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
	char *p;
	int len;

	p = titlePos( buf, &len );
	if( p )
	{
	   if( (int)strlen( section ) == len && strncmp( section, p, len ) == 0 )
		   return true;
	}
	return false;
}

bool TIniFile::gotoSection(const char *section )
{
/*
	功能：移动文件指针到指定section的开始处
	参数：
		section[IN]：要查找的section
	返回值：
		找到并定位返回true
		否则返回false
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
	char line[1024];
   fseek(fin,0,SEEK_SET);
   
	while( FGetS(line,1024,fin)!=NULL)
   {      
	   if( containTitle( line, section ) )
		   return true;
   }
	return false;
}

char *TIniFile::textPos( char *buf, const char *entry )
{
/*
	功能：移动指针到相应的entry处
	参数：
		buf[IN]：源串
		entry[IN]：要查找的entry
	返回值：
		找到并定位返回指向该entry起始的指针
		以';'注释或者未找到返回0
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
   char tmp[1024];
   int i;
	if( buf[0] == ';' ) // it fis comment line
		return 0;


   for (i=0;buf[i];i++)
   {
      if (buf[i]=='=') 
         break;
      tmp[i]=buf[i];
   }
   if (buf[i]!='=')
      return(0);

   tmp[i]=0;

   mytrim(tmp);

	if( strcmp( tmp, entry) == 0 )
		return buf+i+1;

	return 0;
}

void TIniFile::stripQuotationChar( char *buf )
{
/*
	功能：去除字符串前的空格
	参数：
		buf[IN]：源串
	返回值：无
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
	char *p;
	char *q;

	p = buf;
	while( *p && isspace(*p) ) p++;

	if( !(*p == '\"' || *p == '\'') )
		return;

	q = p+strlen(p);
	while( *q != *p && q > p ) q--;
	if( q == p )
		return;
	int len = int(q - p - 1);
	memmove( buf, p+1, len );
	buf[len] = 0;
}

int TIniFile::readEntry(const char *entry, char *buf, int bufSize,bool strip )
{
/*
	功能：读取entry的值
	参数：
		entry[IN]：entry名称
		buf[IN]：存放entry的值的缓冲区
		bufSize[IN]：缓冲区长度
		strip[IN]：去除空格标志
	返回值：
		成功返回读取的长度
		失败返回-1
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
	char lineBuf[1024];
	char *p, *cur;
	int  len;

	cur  = buf;
	*cur = '\0';
	len  = -1;
	while(FGetS(lineBuf,1023,fin))
	{
		if( isTitleLine( lineBuf ) )       // section fis ended
			break;

		p = textPos( lineBuf, entry );     // not equal this entry
		if( p == 0 )
			continue;

		if( strip )
			stripQuotationChar( p );

		len = strlen(p);
		if( bufSize-1 < len )
			len = bufSize-1;

		strncpy( cur, p, len );
		cur[len] = 0;
		break;
	}

	return len;
}

int TIniFile::ReadString( const char *section,const char *entry,const char *defaultString,char *buffer,int   bufLen)
{
/*
	功能：读取指定的section的指定entry的值到buffer中，未找到则使用缺省配置
	参数：
		section[IN]：指定的section
		entry[IN]：指定的entry
		defaultstring[IN]：缺省配置
		buffer[IN]：存放entry值的缓冲区
		bufLen[IN]：缓冲区长度
	返回值：
		成功返回buffer中的值的长度
		失败返回-1
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/

	int len = -1;

   fseek(fin,0,SEEK_SET);
	if( gotoSection( section ) )
		len = readEntry(entry, buffer, bufLen, true);
	percolate(buffer);
	len = strlen(buffer);

	if( len <= 0 ) //can not read entry, use default string
	{
		strncpy( buffer, defaultString, bufLen-1 );
		buffer[bufLen-1] = 0;
		len = strlen(buffer);
	}
	return len;
}

int TIniFile::ReadInt( const char *section,const char *entry,int defaultInt)
{
/*
	功能：读取指定的section的指定entry的值以整数形式返回，未找到则使用缺省配置
	参数：
		section[IN]：指定的section
		entry[IN]：指定的entry
		defaultInt[IN]：缺省配置
	返回值：
		没有找到指定entry则返回缺省值
		否则返回相应的配置值
	修改者：吴舸 2001-04-20
	修改原因：规范编码
*/
	char buf[1024];

   fseek(fin,0,SEEK_SET);
	if( gotoSection( section ) )
	{
		ReadString( section, entry,"-", buf, 1024);
		if (buf[0] != '-') defaultInt = atoi(buf);
	}
	return defaultInt;
}


char * TIniFile::percolate(char *str)
{
	int i,len;
   char *pc;
	len=strlen(str);
	for(i=len-1;i>=0;i--)
	{
		if(str[i]<20)
			str[i]=0;
		else
			break;
	}
//    for (i=0;str[i]!=0;i++)
//    {
//       if (str[i]=='/' && str[i+1]=='/')
//       {
//          str[i]=0;
//          break;
//       }
//    }
   for (pc=str;*pc!=0;pc++)
   {
      if (*pc!=' '&&*pc!='\t')
         break;
   }
	return pc;
}

void TIniFile::in_close()
{
   if (fin!=NULL)
   {
      fclose(fin);
      fin = NULL;
   }
}

void TIniFile::out_close()
{
   if (fout!=NULL)
   {
      fclose(fout);
      fout = NULL;
   }
}

char * TIniFile::FGetS(char *pbuf, int size, FILE *fp)
{
   char tmp[1024];
   char *pc;
   if (fgets(tmp,1023,fp)==NULL)  
      return(NULL);
   pc = percolate(tmp);
   if (pc==NULL) 
   {
      pbuf[0]=0;
   }
   else
   {
      strncpy(pbuf,pc,size);
   }
   return(pbuf);
}


// 函数名: TIniFile::LRTrim
// 编程  : 陈永华 2004-5-7 20:41:24
// 描述  : 从str中将左右空格和'\t'截取掉
// 返回  : char * 
// 参数  : char *str
char * TIniFile::LRTrim(char *str)
{
   char * tmp;
   char * tmp2;
   if(str==NULL)
   {
      return NULL;
   }
   tmp = str;
   while (*tmp)	tmp++;
   if(*str) tmp--;
   while (*tmp == ' '||*tmp=='\t') tmp--;
   *(tmp+1) = 0;
   tmp = str;
   tmp2 = str;
   while ( *tmp2 == ' '||*tmp2=='\t') tmp2 ++ ;
   while ( *tmp2 != 0 ) {
      *tmp = *tmp2; tmp ++; tmp2 ++;
   }
   *tmp = 0;
   return str;   
}


// 函数名: TIniFile::ReadTString
// 编程  : 陈永华 2004-5-7 20:40:18
// 描述  : 新增的读取定义字段的字符串，且过滤左右空格和'\t'
// 返回  : char * 
// 参数  : const char *section
// 参数  : const char *entry
// 参数  : const char *defaultString
// 参数  : char *buffer
// 参数  : int bufLen
char * TIniFile::ReadTString(const char *section, const char *entry, const char *defaultString, char *buffer, int bufLen)
{
   ReadString(section,entry,defaultString,buffer,bufLen);
   return (LRTrim(buffer));
}


void convertPath(char *target, const char *source)
{
	const char *s;
	char *t;
	for (s=source, t=target; ((s-source)<MAX_PATH_LEN) && (*s!='\0'); s++, t++)
	{
		if (strchr(ALL_SPLITS,*s)!=NULL)
		{
			*t=PATH_SPLIT;
		}
		else
		{
			*t=*s;
		}
	}
	*t='\0';
}
	
FILE *mfopen(const char *filename, const char *mode)
{
	char actualName[MAX_PATH_LEN+1];
	convertPath(actualName,filename);
	return fopen(actualName,mode);
}

