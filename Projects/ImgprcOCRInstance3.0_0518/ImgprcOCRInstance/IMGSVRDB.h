#pragma once
#import "C:\\Program Files\\Common Files\\System\\ado\\msado15.dll" \
	no_namespace \
	rename ("EOF","EndOfFile")
#define IS_CHARARRAY_MAXLEN		100
#define IMGSVRDB_RECORDNOTINDB	-2
#define IMGSVRDB_DBERROR		-1
#define IMGSVRDB_RETURN_OK		1

#define IMGSVRDB_RECONECTED		4
#define IMGSVRDB_CONN_CLOSED		-6
#define IMGSVRDB_CONN_NOTCREATED  -7

#include "imgprcTask.h"
class IMGSVRDB
{
public:
	public:
	_ConnectionPtr m_ptrConn;
	_RecordsetPtr m_pSet;
	_RecordsetPtr m_pSet1;
	char m_chsUser[IS_CHARARRAY_MAXLEN];
	char m_chsPassword[IS_CHARARRAY_MAXLEN];
	char m_chsDBName[IS_CHARARRAY_MAXLEN];
	char m_chsDataSourceIP[IS_CHARARRAY_MAXLEN];
	_bstr_t m_bstrErrDsp;
	
	_bstr_t m_connstr;

public:
	IMGSVRDB(void);
	~IMGSVRDB(void);
public:
	int InitiDB(char * sUser, char * sPassword, char * sDatabaseName, char * sDataSourceIP);
//	int AddMailData2DB(MAILDATA *pMail,DWORD dwImageNo);
//	int GetImageInfo(MAILDATA *pMD, DWORD dwImageNo);
//	int DeleteRecord(DWORD dwKillTime);
//	int UpdateMailRecordXML(MAILDATA * pMail, DWORD dwImageNo);
	// 更新背景删除的结果
//	int UpdateMailRecordBR(MAILDATA * pMail, DWORD dwImageNo);
	// //获取邮件的OBR1的条码
//	int GetImageInfo2(MAILDATA * pMail);
	// 确认连接是否成功
	int CheckConnection(void);
	// 使用绑定变量来获取邮件信息
	int bvGetImageInfo(ImgprcTask * pTask);
	
	int bvUpdateImgprc(ImgprcTask * pTask);
	// 使用绑定变量更新XML内容
//	int bvUpdateMailRecordXML(MAILDATA * pMail , DWORD dwImageNo);
	// 使用绑定变量更新图像
//	int bvUpdateMailRecordBR(MAILDATA * pMail, DWORD dwImageNo);
};
