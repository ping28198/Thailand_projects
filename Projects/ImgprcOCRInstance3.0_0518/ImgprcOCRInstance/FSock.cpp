// FSock.cpp
//

#include "stdafx.h"
#include "FSock.h"
#include <afxcoll.h>
#include <windowsx.h>

//******************注意说明******************************//
//Socket 连接类是基于窗口的类，如果没有窗口就会出现未知的错误
//基于窗口就是调用Start函数或者connect函数的地方必须是在
//CWnd类或者CWnd继承的类里面调用

void FSock::Start()
{
	int ret;
	if(bType)
	{
		pVisit=new CMutex(FALSE,NULL,NULL);

		ret=Create(wLocalPort,SOCK_STREAM,NULL);
		
		if(ret==0)
		{
			iLastSystemError=CSocket::GetLastError();
			wLastError=1;
			return;
		}
		ret=Listen();
		if(ret==0)
		{
			iLastSystemError=CSocket::GetLastError();
			wLastError=3;
			return;
		}
	}
	else
	{
		ret=Create(wLocalPort,SOCK_STREAM,LocalIP.GetBuffer(0)); //tx20170913
		if(ret==0)
		{
			iLastSystemError=CSocket::GetLastError();
			wLastError=2;
			return;
		}
		ret=Connect(ServerIP.GetBuffer(0),wRemotePort);
		if(ret==0)
		{
			iLastSystemError=CSocket::GetLastError();
			wLastError=4;
			return;
		}
	}
	bServerClosed=0;
	bRunning=1;
}

void FSock::Stop()
{
	if(bType)
	{
		CloseAllClient();
		Close();
	}
	else
	{
		Close();
	}
	bRunning=0;
}

void FSock::SetType(BOOL bServerClient)
{
	bType=bServerClient;
	if(bType==0)
		wLocalPort=0;
}

void FSock::SetLocalPort(WORD wlocalport)
{
	wLocalPort=wlocalport;
}

void FSock::SetRemotePort(WORD wremoteport)
{
	wRemotePort=wremoteport;
}

void FSock::SetServerIP(CString msIP)
{

	int lLen;

	if(msIP.IsEmpty())
	{
		wLastError=11;
		return;
	}

	if((lLen=msIP.GetLength())>128)
	{
		wLastError=10;
		return;
	}

	ServerIP=msIP;
}
void FSock::SetLocalIP(CString cstrIP)
{
	int lLen;

	if(cstrIP.IsEmpty())
	{
		wLastError=11;
		return;
	}

	if((lLen=cstrIP.GetLength())>128)
	{
		wLastError=10;
		return;
	}

	LocalIP=cstrIP;
}
WORD FSock::GetLastError()
{
	WORD   wLast=wLastError;
    wLastError=0;
	return wLast;
}

int FSock::GetLastSystemError()
{
	int   iLast=iLastSystemError;
    iLastSystemError=0;
	return iLast;
}

CPtrList * FSock::GetListenList()
{
    return m_connectionList;
}

DWORD FSock::GetEndRec()
{
	return dwEndRec;
}

BOOL FSock::GetType() //bType=1 为服务器端 * bType=0 为客户端
{
	return bType;
}

CString FSock::GetServerIP()
{
	return ServerIP;
}

void FSock::OnAccept(int nErrorCode)
{
	BOOL B;
	POSITION     pos;
	int ret;
	unsigned int port;
	FSock  *m_pSock=new  FSock ;
    SOCK_Data  *Sock_Data=new SOCK_Data;

	ret=Accept(*m_pSock);
	if(ret==0)
	{
		iLastSystemError=CSocket::GetLastError();
		wLastError=5;
		delete m_pSock;
		return;
	}

	m_pSock->m_pSockListen=this;
	m_pSock->GetPeerName(Sock_Data->PeerIP,port);
	Sock_Data->m_pSock=m_pSock;
	Sock_Data->wMachineID=0;
	Sock_Data->dwChecker=0;	


	if(m_connectionList==NULL)
	{
		m_connectionList=new (CPtrList);
	}

//	m_connectionList->AddHead(Sock_Data);

	B=pVisit->Lock();
	pos=m_connectionList->GetHeadPosition();
	m_connectionList->AddHead(Sock_Data);
	pos=m_connectionList->GetHeadPosition();
	B=pVisit->Unlock();
		

	CSocket::OnAccept(nErrorCode);
}

void FSock::OnClose(int nErrorCode)
{
	POSITION     pos;
	SOCK_Data  *SockData;
	CPtrList    *list=NULL;
	
	CSocket::OnClose(nErrorCode);

	if(this->m_pSockListen!=NULL)     //for server part, data socket
	{
		list=this->m_pSockListen->m_connectionList;
		this->m_pSockListen->pVisit->Lock();

		for( pos = list->GetHeadPosition(); pos != NULL; )
		{    
			SockData=(SOCK_Data *)list->GetAt(pos);
			if(SockData->m_pSock==this)
			{
				SockData->m_pSock->Close();
				list->RemoveAt(pos);
				break;
			}
			list->GetNext(pos);
		}
		this->m_pSockListen->pVisit->Unlock();

	}

	bServerClosed=1;
}

void FSock::OnReceive(int nErrorCode)
{
	static BYTE  bRecBuf[8*1024];
	static WORD  wRecLen,i;

	wRecLen=Receive(bRecBuf,8*1024);

	doBytes+=wRecLen;


	EnterCriticalSection(&this->crt_section);
	for(i=0;i<wRecLen;i++)
	{
		RecBuf[(dwEndRec+i)%RECEIVE_BUFFER_LEN]=bRecBuf[i];
	}
	dwEndRec=(dwEndRec+wRecLen)%RECEIVE_BUFFER_LEN; //此处不用取模 )%RECEIVE_BUFFER_LEN;//2019-9-19,不取模溢出了怎么办????
	LeaveCriticalSection(&this->crt_section);


	CSocket::OnReceive(nErrorCode);
}
void FSock::OnConnect( int nErrorCode )
{
	bServerClosed=0;
}
void FSock::OnSend(int nErrorCode)
{
}



void FSock::CloseAllClient()
{
	POSITION     pos;	
	SOCK_Data  *Sock_Data;
	
	if(m_connectionList!=NULL)
	{
		pVisit->Lock();
		for( pos = m_connectionList->GetHeadPosition(); pos != NULL; )
		{    
			Sock_Data=(SOCK_Data *)m_connectionList->GetAt(pos);
			Sock_Data->m_pSock->Close();
			m_connectionList->RemoveHead();
			pos=m_connectionList->GetHeadPosition();
		}
		pVisit->Unlock();
	}
}

BOOL  FSock::GetServerClosed()
{
	return  bServerClosed;
}

double FSock::GetCount()
{
	return doBytes;
}

size_t FSock::GetReceivedData(BYTE* pDstBuffer, size_t buffer_length)
{
	size_t get_count = 0;
	size_t stored_length = 0;
	EnterCriticalSection(&this->crt_section);
	if (dwEndRec<dwStartRec)
	{
		stored_length = RECEIVE_BUFFER_LEN + dwEndRec - dwStartRec;
	}
	else
	{
		stored_length = dwEndRec - dwStartRec;
	}
	if (stored_length !=0)
	{
		get_count = (buffer_length < stored_length) ? buffer_length : stored_length;
		for (int i=0;i<get_count;i++)
		{
			pDstBuffer[i] = RecBuf[(dwStartRec+i)%RECEIVE_BUFFER_LEN];
		}
		dwStartRec = (dwStartRec + get_count) % RECEIVE_BUFFER_LEN;
	}
	LeaveCriticalSection(&this->crt_section);
	return get_count;
}

BOOL FSock::IsRunning()
{
	return bRunning;
}

FSock::FSock()
{
	wLocalPort=0x80;
	wRemotePort=0x80;
	m_connectionList=NULL;
	m_pSockListen=NULL;
	dwEndRec=dwStartRec=0;
	bType=0;
	bRunning=0;
	doBytes=0;
	bServerClosed=0;
	wLastError=iLastSystemError=0;
	pVisit=NULL;
	InitializeCriticalSection(&this->crt_section);
}

FSock::~FSock()
{
	if(bRunning) Stop();
	if(m_connectionList) delete(m_connectionList);
	if (pVisit) delete pVisit;
	WSACleanup();
}


