// FSock.h
//

#if !defined(FSock_h)
#define FSock_h

/*
 * ����̳���CSock�ࡣΪʹ�ü��ش˷�װ��
 * typedef struct{
 * FSock *m_pSock;  //��������
 * CString PeerIP;   //�Է���IP��ַ
 * unsigned int wMachineID; //����ID
 * DWORD dwChecker;//ʱ�Ӽ������
 * }SOCK_Data;
 * �����ʹ�÷�����
 * �������˵Ŀ������裺
 * 1.���÷���SetType(1)���ü�����,���÷���  SetLocalPort(wLPort)���ü����˿ڡ�
 * 2.���÷���Start().���GetLastError()�����޴�
 *   �������������óɹ���
 * 3.���÷��������Ա����������������ࡣ
 *   m_connectionList=sockListen.GetListenList();
 *   if(m_connectionList)
 *   {
 *     for( pos =          m_connectionList->GetHeadPosition();
 *     pos != NULL; )
 * {
 *    m_pSock_Data=(SOCK_Data        *)m_connectionList->GetAt(pos);
 *    //����m_pSock_Data->m_pSock
 *    m_connectionList->GetNext(pos);
 * }
 * }
 * 3.1
 *    ����m_pSock. ͨ�����ʶ���   RecBuf[RECEIVE_BUFFER_LEN]��ָ��dwEndRec����
 *    �÷��ʽ��յ����ݣ�����dwStartRec��������ʹ�á�
 * 3.2
 *    ͨ������m_pSock->Send(BYTE *Buffer,int nSendLen,0)�������ݡ�
 * 4.���÷���Stop()�رռ����ࡣ
 * 
 * �ͻ��˵Ŀ������裺
 * 1.���÷���SetType(0)���ÿͻ������࣬�÷���SetServerIP,����SERVER
 * ��IP��ַ������������÷���SetRemotePort����SERVER�ļ����˿ڡ�
 * 2.���÷���Start().���GetLastError()�����޴�
 *   �������ӳɹ���
 * 3.ͨ�����ʶ���RecBuf[RECEIVE_BUFFER_LEN]��ָ��dwEndRec���Է��ʽ��յ����ݣ�����dwStartRec��������ʹ�á�
 * 4 ͨ������Send(BYTE *Buffer,int nSendLen,0)�������ݡ�
 * 5.���÷���Stop()�رտͻ������ࡣ
 * 
 * ע�⣺
 * ����RECEIVE_BUFFER_LENΪ���г��ȡ�
 * ����#define RECEIVE_BUFFER_LEN  (60*1024)
 * 
 * wLastErro �������£�
 * 1  ��������Create()����WINSOCK������ʧ�ܡ�
 * 2  �ͻ�  ��Create()����WINSOCK������ʧ�ܡ�
 * 3  ��������Listen()           ������ʧ�ܡ�
 * 4  �ͻ�  ��Connect()          ������ʧ�ܡ�
 * 5  ��������Accept()           ������ʧ�ܡ�
 * 10 ����SetServerIP()���ò���Ϊ�ա�
 * 11 ����SetServerIP()���ò�����ָ����ַ������ȴ���128�ֽڡ�
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
     * ��ʼ����WINSOCK��
     * ����Ƿ�����������������������
     * ����ǿͻ���  �������������ӡ�
     */
    void Start();
    /*
     * ��ʼ�ر�WINSOCK��
     * ����Ƿ������ˣ��رռ�����
     * ����ǿͻ���  ���ر����ӡ�
     */
    void Stop();
    /*
     * ����WINSOCK���͡�
     * ������
     *      bServerClient  =1    Ϊ��������
     *      bServerClient  =0    Ϊ�ͻ���
     *      
     */
    void SetType(BOOL bServerClient);
    /*
     * ���ü����˿ڣ���˽������wLocalPort.
     */
    void SetLocalPort(WORD wLPort);
    /*
     * �ͻ�������Զ�̶˿ڣ���ͬ�������ļ����˿ڡ�
     */
    void SetRemotePort(WORD wRPort);
    /*
     * ����SERVER IP����˽������CString ServerIP.
     * ������
     *      CString SIP;�ȿ����� IP ��ַ��Ҳ������ SERVER �Ļ�������
     */
    void SetServerIP(CString SIP);
	//���ÿͻ���IP
	void SetLocalIP(CString LIP);  //tx20170913
    /*
     * ����˽������wLastError��ֵ������ʹ�ù����еĳ�����Ϣ��
     */
    WORD GetLastError();
    /*
     * ����˽������iLastSystemError��ֵ������ʹ�ù����еĳ�����Ϣ��
     */
    int GetLastSystemError();
    /*
     * ����˽������m_connectionList��ֵ
     */
    CPtrList * GetListenList();
    /*
     * ����˽������dwEndRec��ֵ��
     */
    DWORD GetEndRec();
	
    /*
     * ����˽������wLocalPort��ֵ��
     */
    WORD GetLocalPort();
    /*
     * ����˽������bType��ֵ��
     */
    BOOL GetType();
    /*
     * ����˽������wRemotePort��ֵ��
     */

    WORD GetRemotePort();
    /*
     * ����˽������ServerIP��ֵ��
     */
    CString GetServerIP();

	BOOL  GetServerClosed();
    virtual void OnAccept(int nErrorCode);
    virtual void OnClose(int nErrorCode);
    virtual void OnReceive(int nErrorCode);
    virtual void OnSend(int nErrorCode);
	virtual void OnConnect( int nErrorCode );

    /*
     * ����˽������doBytes��ֵ��
     */
    double GetCount();
    /*
     * ����˽������bRunning��ֵ��
     */
	size_t GetReceivedData(BYTE* pDstBuffer, size_t buffer_length); //�̰߳�ȫ




    BOOL IsRunning();
    FSock();
    ~FSock();


	/*
     * �ڼ������У���������TCP/IP���ӵĻ����ź�����
     */
    CMutex	*pVisit;

protected:
private:
	/*
 * ���ڽ������ݵĶ��С�
 */
	BYTE	RecBuf[RECEIVE_BUFFER_LEN];//��Ҫ����������
/*
 * �û�������ն��еĴ���ָ�롣
 */
	DWORD	dwStartRec;

	//�ٽ�����ֹ��ͻ
	CRITICAL_SECTION crt_section;

    /*
     * �������˵���ر�ʱ���ȹر���֮������SOCKET��
     */

    void CloseAllClient();
    /*
     * ָ��WINSOCK�����͡�
     * bType=1 Ϊ��������
     * bType=0 Ϊ�ͻ���
     */
    BOOL	bType;
    /*
     * �����˿�
     */
    WORD	wLocalPort;
    /*
     *  �ͻ����ӣ��������������˿ڡ�
     */
    WORD	wRemotePort;
    /*
     * SERVER �� IP ��ַ ���������
     */
    CString	ServerIP;
    /*
     * ���ڼ�¼������ʹ�ù����еĴ���
     * �Լ�����ĳ�����Ϣ����ϸ��Ϣ��ʵ�����ĵ���
     */
	CString LocalIP; //tx20170913
    WORD	wLastError;
    /*
     * ���ڼ�¼������ʹ�ù����е�CSock����
     * ������Ϣ��μ�MSDN��
     */
    int	iLastSystemError;
    /*
     * ���ն��еĽ���ָ�롣
     */
    DWORD	dwEndRec; //ָ����ĩβ��λ������
    /*
     * ���ڼ�¼��������������ӵĿͻ������ӡ�
     */
    CPtrList *	m_connectionList;
    /*
     * ���ڼ������У����ɿͻ��������������ʱ�����ɵ�������ָ���������ļ����ࡣ
     */
    FSock *	m_pSockListen;
    /*
     * ���ڼ�¼��SOCK�����յ��ַ�����
     */
    double	doBytes;
    /*
     * ���ڼ�¼�ͻ��˻���������������
     * 1=����
     * 2=δ������
     */
    BOOL	bRunning;
	BOOL    bServerClosed;


};

typedef struct{
	FSock *m_pSock;  //��������
	CString PeerIP;   //�Է���IP��ַ
	unsigned int wMachineID; //����ID
	DWORD dwChecker;//ʱ�Ӽ������
}SOCK_Data;

#endif /* FSock_h */
