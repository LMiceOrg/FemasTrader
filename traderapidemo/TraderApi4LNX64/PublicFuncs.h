// PublicFuncs.h: interface for the PublicFuncs class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PUBLICFUNCS_H__F565A242_2630_4908_B2B8_011FDAA45C89__INCLUDED_)
#define AFX_PUBLICFUNCS_H__F565A242_2630_4908_B2B8_011FDAA45C89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
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
#include <algorithm>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <queue>
#include <string>
#include <stack>
#include "USTPFtdcTraderApi.h"
#include "USTPFtdcUserApiStruct.h"
#ifdef WIN32
#include <windows.h>
typedef HANDLE THREAD_HANDLE ;
#else
#include <pthread.h>
typedef pthread_t THREAD_HANDLE ;
#endif
const int BUFLEN=512;

class PublicFuncs  
{
public:
	PublicFuncs();
	virtual ~PublicFuncs();
};

extern TUstpFtdcBrokerIDType g_BrokerID;
extern TUstpFtdcUserIDType	g_UserID;
extern TUstpFtdcPasswordType	g_Password;
extern char g_frontaddr[BUFLEN];
extern int g_nOrdLocalID;
extern char* g_pProductInfo;
extern CUstpFtdcTraderApi * g_puserapi;
extern bool StartAutoOrder();
extern void StartTest();

#endif // !defined(AFX_PUBLICFUNCS_H__F565A242_2630_4908_B2B8_011FDAA45C89__INCLUDED_)
