/********************************************************
	��ȡ�����ļ���ʵ��
	�ļ���: profile.cpp
	˵��: 
	����: 
	�޸�: ���� 2001-04-20
	�޸�ԭ��: �淶����
**********************************************************/
#include "profile.h"

/* ȥ�����ҿո�   */
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
	���ܣ��ر����������
	��������
	����ֵ����
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
*/
   fin = NULL;
   fout = NULL;   
}

TIniFile::~TIniFile()
{
/*
	���ܣ��ر����������
	��������
	����ֵ����
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
*/
   Close();   
}
void TIniFile::Close()
{
/*
	���ܣ��ر����������
	��������
	����ֵ����
	�޸��ߣ����� 2001-06-07
	�޸�ԭ�򣺹淶����
*/
   in_close();
   out_close();
}

bool TIniFile::Open(char *filename)
{
/*
	���ܣ��������ļ�
	������
		filename[IN]�������ļ���
	����ֵ��
		�ɹ�����true
		ʧ�ܷ���false
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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
	���ܣ�ȡ��ָ��section�Ŀ�ʼ��ָ�룬����buf�е�ƫ����
	������
		buf[IN]��Դ��
		len[OUT]��ƫ����
	����ֵ��
		��һ��section�򷵻�ָ���section��ʼ��ָ��
		���򷵻�0
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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
	���ܣ��ַ������Ƿ���һ��section
	������
		bufPtr[IN]��Դ��
	����ֵ��
		�ɹ�����true
		ʧ�ܷ���false
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
*/
	return titlePos( bufPtr, 0 ) != 0;
}

bool TIniFile::containTitle( char *buf, const char *section )
{
/*
	���ܣ��ַ������Ƿ����ָ����section
	������
		buf[IN]��Դ��
		section[IN]��Ҫ���ҵ�section
	����ֵ��
		�ҵ�����true
		���򷵻�false
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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
	���ܣ��ƶ��ļ�ָ�뵽ָ��section�Ŀ�ʼ��
	������
		section[IN]��Ҫ���ҵ�section
	����ֵ��
		�ҵ�����λ����true
		���򷵻�false
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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
	���ܣ��ƶ�ָ�뵽��Ӧ��entry��
	������
		buf[IN]��Դ��
		entry[IN]��Ҫ���ҵ�entry
	����ֵ��
		�ҵ�����λ����ָ���entry��ʼ��ָ��
		��';'ע�ͻ���δ�ҵ�����0
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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
	���ܣ�ȥ���ַ���ǰ�Ŀո�
	������
		buf[IN]��Դ��
	����ֵ����
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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
	���ܣ���ȡentry��ֵ
	������
		entry[IN]��entry����
		buf[IN]�����entry��ֵ�Ļ�����
		bufSize[IN]������������
		strip[IN]��ȥ���ո��־
	����ֵ��
		�ɹ����ض�ȡ�ĳ���
		ʧ�ܷ���-1
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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
	���ܣ���ȡָ����section��ָ��entry��ֵ��buffer�У�δ�ҵ���ʹ��ȱʡ����
	������
		section[IN]��ָ����section
		entry[IN]��ָ����entry
		defaultstring[IN]��ȱʡ����
		buffer[IN]�����entryֵ�Ļ�����
		bufLen[IN]������������
	����ֵ��
		�ɹ�����buffer�е�ֵ�ĳ���
		ʧ�ܷ���-1
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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
	���ܣ���ȡָ����section��ָ��entry��ֵ��������ʽ���أ�δ�ҵ���ʹ��ȱʡ����
	������
		section[IN]��ָ����section
		entry[IN]��ָ����entry
		defaultInt[IN]��ȱʡ����
	����ֵ��
		û���ҵ�ָ��entry�򷵻�ȱʡֵ
		���򷵻���Ӧ������ֵ
	�޸��ߣ����� 2001-04-20
	�޸�ԭ�򣺹淶����
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


// ������: TIniFile::LRTrim
// ���  : ������ 2004-5-7 20:41:24
// ����  : ��str�н����ҿո��'\t'��ȡ��
// ����  : char * 
// ����  : char *str
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


// ������: TIniFile::ReadTString
// ���  : ������ 2004-5-7 20:40:18
// ����  : �����Ķ�ȡ�����ֶε��ַ������ҹ������ҿո��'\t'
// ����  : char * 
// ����  : const char *section
// ����  : const char *entry
// ����  : const char *defaultString
// ����  : char *buffer
// ����  : int bufLen
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

