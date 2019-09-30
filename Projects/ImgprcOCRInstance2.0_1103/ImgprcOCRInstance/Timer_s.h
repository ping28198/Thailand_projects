// Timer.h
//

#if !defined(Timer_h)
#define Timer_h

#include "stdafx.h"
#include <mmsystem.h>

/*
 * ����Ϊͨ��ʱ���ࡣ���ȱʡ����һ���ظ��ж�ʱ�ӡ�
 * 
 * ����ΪStar()��ʼһ��ʱ���ж��¼���
 * ����ΪStop()ֹͣһ��ʱ���ж��¼���
 * 
 * �����ʹ�÷�����
 * 
 * 1.����Ĺ��캯�������ж��ӳ٣��ж����ͣ��û�ID��
 * 2.����AddTimerListenerΪʱ�Ӱ�װ��������
 * 3.�÷���Start()��ʼһ��ʱ���жϡ�
 * 4.�÷���Stop() ����һ��ʱ���жϡ�
 * 
 * 
 * ����
 * 1.����Ĺ��캯�� Timer(long p,int  id,BOOL r,LPTIMECALLBACK
 * fservice)��ʼ����;
 * 2.�÷���Start()��ʼһ��ʱ���жϡ�
 * 3.�÷���Stop() ����һ��ʱ���жϡ�
 * 
 */

#include "resource.h"


class Timer
{
public:

    /*
     * ���캯����
     * ������
     *      long p;
     *      int  id;
     *      BOOL r;   r=1 Ϊ�ظ��ж�ʱ�� r=0Ϊһ�����жϡ�
     *      LPTIMECALLBACK fservice;
     */
    Timer(long p, int id, BOOL r, LPTIMECALLBACK fservice);
    /*
     * ���캯��,��ʱ���жϷ�������ɷ���AddTimerListener()��װ��
     * ������
     *      long p;
     *      int  id;
     *      BOOL r;
     *      
     */
    Timer(long p, int id, BOOL r);
    /*
     * ���캯��,��ʱ���жϷ�������ɷ���AddTimerListener()��װ��
     * ������
     *      long p;
     *      int  id;
     */
    Timer(long p, int id);
    /*
     * ���캯��,��ʱ���жϷ�������ɷ���AddTimerListener()��װ��
     * ������
     *      long p;
     */
    Timer(long p);
    /*
     * �����û�ID,��˽������wUserID��ֵ��
     */
    WORD GetUserID();
    /*
     * ����ϵͳID,��˽������uSystemID��ֵ��
     */
    UINT GetSystemID();
    /*
     * ����ʱ������,��˽������bRepeat��ֵ��
     */
    BOOL IsRepeat();
    /*
     * ����ʱ���Ƿ�����,��˽������bRunning��ֵ��
     */
    BOOL IsRunning();
    /*
     * Ϊʱ���жϰ�װ�жϷ������
     * fserviceָ���жϷ������
     * 
     * ����ֵ0 ���� �ɹ���
     *      ~0 ���� ʧ�ܡ�
     */
    int AddTimerListener(LPTIMECALLBACK fservice);
    /*
     * ����ʱ������,��˽������wPeroid��ֵ��
     */
    long GetPeroid();
    /*
     * ���ü�����ʱ�ӡ�
     * ����ֵ~0  ��ʾ�ɹ���
     *       0   ��ʾ���ɹ���
     */
    //UINT Start(DWORD dwUser=0xffffffff);
	UINT Start(DWORD_PTR  dwUser);
    /*
     * ֹͣʱ�ӡ�
     */
    int Stop();
    /*
     * ����������ɾ����ʱ���¼���ɾ��Ӧ�ó���������С��ʱ�����ȡ�
     */
    ~Timer();
    /*
     * ����˽������wLastError��ֵ��
     */
    WORD GetLastError();

protected:


private:

    /*
     * ���캯����ͬ�ĳ�ʼ������
     */
    void Init();
    /*
     * ����ϵͳ����Сʱ�Ӿ��ȣ���˽������wTimerRes�С�
     */
    WORD GetResolution();
    /*
     * ��Сʱ�Ӿ��ȡ������뵥λ��
     */
    WORD	wTimerRes;
    /*
     * ���û���װ���жϷ������
     */
    LPTIMECALLBACK	lpFunction;
    /*
     * =1 Ϊ�ظ��ж�ʱ��   =0Ϊһ�����жϡ�
     */
    BOOL	bRepeat;
    /*
     * ���û����õ�ʱ��ID��
     */
    WORD	wUserID;
    /*
     * ʱ��   ϵͳID��
     */
    UINT	uSystemID;
    /*
     * ʱ�����б�־��
     */
    BOOL	bRunning;
    /*
     * ʱ�����ڡ�
     */
    long	lPeriod;
    /*
     * ��¼������ʹ�ù����У�������Ϣ��
     * ������Ϣ�����ʵ���ߵ���ϸ�ĵ���
     * wLastError��ֵ���塣
     * =1 timeGetDevCaps��������ʧ��(ȡϵͳ��С����)
     * =2 timeSetEvent  ��������ʧ��(����ʱ���¼�)
     * =3 timeKillEvent ��������ʧ��(ɾ��ʱ���¼�)
     * =4 �ص�����Ϊ�ա�
	 * =5 ʱ���Ѿ����С�
     */
    WORD	wLastError;

};

#endif /* Timer_h */
