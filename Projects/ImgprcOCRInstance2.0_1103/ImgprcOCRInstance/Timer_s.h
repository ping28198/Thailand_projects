// Timer.h
//

#if !defined(Timer_h)
#define Timer_h

#include "stdafx.h"
#include <mmsystem.h>

/*
 * 该类为通用时钟类。类的缺省构造一个重复中断时钟。
 * 
 * 方法为Star()开始一个时钟中断事件。
 * 方法为Stop()停止一个时钟中断事件。
 * 
 * 该类的使用方法：
 * 
 * 1.由类的构造函数设置中断延迟（中断类型，用户ID）
 * 2.方法AddTimerListener为时钟安装处理句柄。
 * 3.用方法Start()开始一个时钟中断。
 * 4.用方法Stop() 结束一个时钟中断。
 * 
 * 
 * 或者
 * 1.由类的构造函数 Timer(long p,int  id,BOOL r,LPTIMECALLBACK
 * fservice)初始化类;
 * 2.用方法Start()开始一个时钟中断。
 * 3.用方法Stop() 结束一个时钟中断。
 * 
 */

#include "resource.h"


class Timer
{
public:

    /*
     * 构造函数。
     * 参数：
     *      long p;
     *      int  id;
     *      BOOL r;   r=1 为重复中断时钟 r=0为一次性中断。
     *      LPTIMECALLBACK fservice;
     */
    Timer(long p, int id, BOOL r, LPTIMECALLBACK fservice);
    /*
     * 构造函数,定时器中断服务程序由方法AddTimerListener()安装。
     * 参数：
     *      long p;
     *      int  id;
     *      BOOL r;
     *      
     */
    Timer(long p, int id, BOOL r);
    /*
     * 构造函数,定时器中断服务程序由方法AddTimerListener()安装。
     * 参数：
     *      long p;
     *      int  id;
     */
    Timer(long p, int id);
    /*
     * 构造函数,定时器中断服务程序由方法AddTimerListener()安装。
     * 参数：
     *      long p;
     */
    Timer(long p);
    /*
     * 返回用户ID,及私有属性wUserID的值。
     */
    WORD GetUserID();
    /*
     * 返回系统ID,及私有属性uSystemID的值。
     */
    UINT GetSystemID();
    /*
     * 返回时钟类型,及私有属性bRepeat的值。
     */
    BOOL IsRepeat();
    /*
     * 返回时钟是否运行,及私有属性bRunning的值。
     */
    BOOL IsRunning();
    /*
     * 为时钟中断安装中断服务程序。
     * fservice指向中断服务程序
     * 
     * 返回值0 设置 成功。
     *      ~0 设置 失败。
     */
    int AddTimerListener(LPTIMECALLBACK fservice);
    /*
     * 返回时钟周期,及私有属性wPeroid的值。
     */
    long GetPeroid();
    /*
     * 设置及开启时钟。
     * 返回值~0  表示成功。
     *       0   表示不成功。
     */
    //UINT Start(DWORD dwUser=0xffffffff);
	UINT Start(DWORD_PTR  dwUser);
    /*
     * 停止时钟。
     */
    int Stop();
    /*
     * 析构函数。删除定时器事件。删除应用程序建立的最小定时器精度。
     */
    ~Timer();
    /*
     * 返回私有属性wLastError的值。
     */
    WORD GetLastError();

protected:


private:

    /*
     * 构造函数共同的初始化代码
     */
    void Init();
    /*
     * 返回系统的最小时钟精度，在私有属性wTimerRes中。
     */
    WORD GetResolution();
    /*
     * 最小时钟精度。（毫秒单位）
     */
    WORD	wTimerRes;
    /*
     * 由用户安装的中断服务程序。
     */
    LPTIMECALLBACK	lpFunction;
    /*
     * =1 为重复中断时钟   =0为一次性中断。
     */
    BOOL	bRepeat;
    /*
     * 由用户设置的时钟ID。
     */
    WORD	wUserID;
    /*
     * 时钟   系统ID。
     */
    UINT	uSystemID;
    /*
     * 时钟运行标志。
     */
    BOOL	bRunning;
    /*
     * 时钟周期。
     */
    long	lPeriod;
    /*
     * 记录该类在使用过程中，出错信息。
     * 错误信息定义见实现者的详细文档。
     * wLastError的值定义。
     * =1 timeGetDevCaps函数调用失败(取系统最小精度)
     * =2 timeSetEvent  函数调用失败(设置时钟事件)
     * =3 timeKillEvent 函数调用失败(删除时钟事件)
     * =4 回调函数为空。
	 * =5 时钟已经运行。
     */
    WORD	wLastError;

};

#endif /* Timer_h */
