// FSock.h
//

#if !defined(FSock_h)
#define FSock_h

/*
 * 该类继承了CSock类。为使用简单特此封装。
 * typedef struct{
 * FSock *m_pSock;  //数据连接
 * CString PeerIP;   //对方的IP地址
 * unsigned int wMachineID; //机器ID
 * DWORD dwChecker;//时钟检验参数
 * }SOCK_Data;
 * 该类的使用方法。
 * 服务器端的开发步骤：
 * 1.调用方法SetType(1)设置监听类,调用方法  SetLocalPort(wLPort)设置监听端口。
 * 2.调用方法Start().如果GetLastError()返回无错。
 *   表明监听类设置成功。
 * 3.调用方法，可以遍历与监听类的连接类。
 *   m_connectionList=sockListen.GetListenList();
 *   if(m_connectionList)
 *   {
 *     for( pos =          m_connectionList->GetHeadPosition();
 *     pos != NULL; )
 * {
 *    m_pSock_Data=(SOCK_Data        *)m_connectionList->GetAt(pos);
 *    //访问m_pSock_Data->m_pSock
 *    m_connectionList->GetNext(pos);
 * }
 * }
 * 3.1
 *    访问m_pSock. 通过访问队列   RecBuf[RECEIVE_BUFFER_LEN]及指针dwEndRec可以
 *    访访问接收的数据，属性dwStartRec给调用者使用。
 * 3.2
 *    通过方法m_pSock->Send(BYTE *Buffer,int nSendLen,0)发送数据。
 * 4.调用方法Stop()关闭监听类。
 * 
 * 客户端的开发步骤：
 * 1.调用方法SetType(0)设置客户连接类，用方法SetServerIP,设置SERVER
 * 的IP地址或机器名，调用方法SetRemotePort设置SERVER的监听端口。
 * 2.调用方法Start().如果GetLastError()返回无错。
 *   表明连接成功。
 * 3.通过访问队列RecBuf[RECEIVE_BUFFER_LEN]及指针dwEndRec可以访问接收的数据，属性dwStartRec给调用者使用。
 * 4 通过方法Send(BYTE *Buffer,int nSendLen,0)发送数据。
 * 5.调用方法Stop()关闭客户连接类。
 * 
 * 注意：
 * 常量RECEIVE_BUFFER_LEN为队列长度。
 * 建议#define RECEIVE_BUFFER_LEN  (60*1024)
 * 
 * wLastErro 定义如下：
 * 1  服务器端Create()创建WINSOCK，调用失败。
 * 2  客户  端Create()创建WINSOCK，调用失败。
 * 3  服务器端Listen()           ，调用失败。
 * 4  客户  端Connect()          ，调用失败。
 * 5  服务器端Accept()           ，调用失败。
 * 10 方法SetServerIP()调用参数为空。
 * 11 方法SetServerIP()调用参数所指向的字符串长度大于128字节。
 */
#include "resource.h"
#include <afx.h>
#include <afxmt.h>
#include <afxsock.h>


class CPtrList;
#define RECEIVE_BUFFER_LEN  (60*1024)

class FSock : public CSocket
{	

public:
    /*
     * 开始启动WINSOCK。
     * 如果是服务器端启动：启动监听。
     * 如果是客户端  启动：启动连接。
     */
    void Start();
    /*
     * 开始关闭WINSOCK。
     * 如果是服务器端：关闭监听。
     * 如果是客户端  ：关闭连接。
     */
    void Stop();
    /*
     * 设置WINSOCK类型。
     * 参数：
     *      bServerClient  =1    为服务器端
     *      bServerClient  =0    为客户端
     *      
     */
    void SetType(BOOL bServerClient);
    /*
     * 设置监听端口，即私有属性wLocalPort.
     */
    void SetLocalPort(WORD wLPort);
    /*
     * 客户端设置远程端口，即同服务器的监听端口。
     */
    void SetRemotePort(WORD wRPort);
    /*
     * 设置SERVER IP，即私有属性CString ServerIP.
     * 参数：
     *      CString SIP;既可以是 IP 地址，也可以是 SERVER 的机器名。
     */
    void SetServerIP(CString SIP);
	//设置客户端IP
	void SetLocalIP(CString LIP);  //tx20170913
    /*
     * 返回私有属性wLastError的值即该类使用过程中的出错信息。
     */
    WORD GetLastError();
    /*
     * 返回私有属性iLastSystemError的值即该类使用过程中的出错信息。
     */
    int GetLastSystemError();
    /*
     * 返回私有属性m_connectionList的值
     */
    CPtrList * GetListenList();
    /*
     * 返回私有属性dwEndRec的值。
     */
    DWORD GetEndRec();
	
    /*
     * 返回私有属性wLocalPort的值。
     */
    WORD GetLocalPort();
    /*
     * 返回私有属性bType的值。
     */
    BOOL GetType();
    /*
     * 返回私有属性wRemotePort的值。
     */

    WORD GetRemotePort();
    /*
     * 返回私有属性ServerIP的值。
     */
    CString GetServerIP();

	BOOL  GetServerClosed();
    virtual void OnAccept(int nErrorCode);
    virtual void OnClose(int nErrorCode);
    virtual void OnReceive(int nErrorCode);
    virtual void OnSend(int nErrorCode);
	virtual void OnConnect( int nErrorCode );

    /*
     * 返回私有属性doBytes的值。
     */
    double GetCount();
    /*
     * 返回私有属性bRunning的值。
     */
	size_t GetReceivedData(BYTE* pDstBuffer, size_t buffer_length); //线程安全




    BOOL IsRunning();
    FSock();
    ~FSock();


	/*
     * 在监听类中，用作遍历TCP/IP连接的互斥信号量。
     */
    CMutex	*pVisit;

protected:
private:
	/*
 * 用于接收数据的队列。
 */
	BYTE	RecBuf[RECEIVE_BUFFER_LEN];//需要被保护起来
/*
 * 用户处理接收队列的处理指针。
 */
	DWORD	dwStartRec;

	//临界区防止冲突
	CRITICAL_SECTION crt_section;

    /*
     * 服务器端的类关闭时，先关闭与之相连的SOCKET。
     */

    void CloseAllClient();
    /*
     * 指明WINSOCK的类型。
     * bType=1 为服务器端
     * bType=0 为客户端
     */
    BOOL	bType;
    /*
     * 监听端口
     */
    WORD	wLocalPort;
    /*
     *  客户连接（服务器监听）端口。
     */
    WORD	wRemotePort;
    /*
     * SERVER 的 IP 地址 或机器名。
     */
    CString	ServerIP;
    /*
     * 用于记录该类在使用过程中的错误。
     * 自己定义的出错信息。详细信息见实现者文档。
     */
	CString LocalIP; //tx20170913
    WORD	wLastError;
    /*
     * 用于记录该类在使用过程中的CSock错误。
     * 出错信息请参见MSDN。
     */
    int	iLastSystemError;
    /*
     * 接收队列的接收指针。
     */
    DWORD	dwEndRec; //指数据末尾空位处索引
    /*
     * 用于记录与监听类类相连接的客户端链接。
     */
    CPtrList *	m_connectionList;
    /*
     * 用于监听类中，当由客户端与服务器连接时，生成的连接类指向生成它的监听类。
     */
    FSock *	m_pSockListen;
    /*
     * 用于记录该SOCK共接收的字符数。
     */
    double	doBytes;
    /*
     * 用于记录客户端或服务器端启动与否。
     * 1=启动
     * 2=未启动。
     */
    BOOL	bRunning;
	BOOL    bServerClosed;


};

typedef struct{
	FSock *m_pSock;  //数据连接
	CString PeerIP;   //对方的IP地址
	unsigned int wMachineID; //机器ID
	DWORD dwChecker;//时钟检验参数
}SOCK_Data;

#endif /* FSock_h */
