// Timer.cpp
/*************************
20130827: Timer::Start()函数添加一个参数用于传递调用线程的对象的指针
Timer::Start()->Timer::Start(DWORD_PTR  dwUser)
************************/


#include "stdafx.h"
#include "Timer.h"
//

Timer::Timer(long p, int id, BOOL r, LPTIMECALLBACK fservice)
{
	Init();
	lPeriod=p;
	wUserID=id;
	bRepeat=r;
	lpFunction=fservice;
}

Timer::Timer(long p, int id, BOOL r)
{
	Init();
	lPeriod=p;
	wUserID=id;
	bRepeat=r;
}

Timer::Timer(long p, int id)
{
	Init();
	lPeriod=p;
	wUserID=id;
}

Timer::Timer(long p)
{
	Init();
	lPeriod=p;
}

void Timer::Init()
{
	WORD wRet;
	lpFunction=NULL;
	bRepeat=TIME_PERIODIC;
	wUserID=0;
	uSystemID=0;
	wLastError=0;
	bRunning=0;
	lPeriod=1000;
	
	wRet=GetResolution();
	if(wRet==0)
	{
		//AfxMessageBox("");
	}
}

WORD Timer::GetUserID()
{
	return wUserID;
}

UINT Timer::GetSystemID()
{
	return uSystemID;
}

BOOL Timer::IsRepeat()
{
    return bRepeat;
}

BOOL Timer::IsRunning()
{
    return bRunning;
}

int Timer::AddTimerListener(LPTIMECALLBACK fservice)
{
	lpFunction=fservice;
    return (int)0;
}

long Timer::GetPeroid()
{
     return (long)lPeriod;
}
//
UINT Timer::Start(DWORD_PTR  dwUser)
{
//	DWORD dwUser=0xffffffff; //tx20130827
	MMRESULT ret;
	UINT     uTimerType;
	
    if(bRunning==1)
	{
		wLastError=5;
		return 0;
	}

    if(lpFunction==NULL)
	{
		wLastError=4;
		return 0;
	}

	ret=timeBeginPeriod(wTimerRes);

	if(bRepeat==1)  uTimerType=TIME_PERIODIC;
	else        	uTimerType=TIME_ONESHOT;
	
	uSystemID=timeSetEvent(lPeriod,wTimerRes,(LPTIMECALLBACK)lpFunction,
		(DWORD_PTR )dwUser,uTimerType);
		
	if(uSystemID==NULL)
	{
		wLastError=2;
		return 0;
	}
	
	bRunning=1;
    return uSystemID;
}

int Timer::Stop()
{
	MMRESULT ret;
	if(uSystemID)
	{		
		ret=timeKillEvent(uSystemID);
		uSystemID=0;
		timeEndPeriod(wTimerRes);
		bRunning=0;

		if(ret==MMSYSERR_NOERROR)
		{
			return 1;
		}

		if(ret==MMSYSERR_INVALPARAM)
		{
           wLastError=3;
		   return 0;
		}

	};
    return (int)1;
	
}

WORD Timer::GetResolution()
{
	TIMECAPS tc;
	if(timeGetDevCaps(&tc,sizeof(TIMECAPS))!=TIMERR_NOERROR)
	{
		//Record Error
		wLastError=1;
		return 0;
	}
    wTimerRes=min(max(tc.wPeriodMin,1),tc.wPeriodMax);
	return wTimerRes;
}

Timer::~Timer()
{
	if(bRunning==1)
	{
		if(uSystemID!=0)  
			timeKillEvent(uSystemID);
		timeEndPeriod(wTimerRes);
	}
}

WORD Timer::GetLastError()
{
	WORD wLast=wLastError;
	wLastError=0;
	return wLast;
}


