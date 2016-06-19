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
   fin = NULL;
   fout = NULL;   
}

TIniFile::~TIniFile()
{
   Close();   
}
void TIniFile::Close()
{
   in_close();
   out_close();
}

bool TIniFile::Open(char *filename)
{
    in_close();
    fin=fopen(filename,"rt");
    return(fin!=NULL);
}
char *TIniFile::titlePos( char *buf, int *len )
{
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
	return titlePos( bufPtr, 0 ) != 0;
}

bool TIniFile::containTitle( char *buf, const char *section )
{
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


char * TIniFile::ReadTString(const char *section, const char *entry, const char *defaultString, char *buffer, int bufLen)
{
   ReadString(section,entry,defaultString,buffer,bufLen);
   return (LRTrim(buffer));
}
