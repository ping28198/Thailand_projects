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
	// ���±���ɾ���Ľ��
//	int UpdateMailRecordBR(MAILDATA * pMail, DWORD dwImageNo);
	// //��ȡ�ʼ���OBR1������
//	int GetImageInfo2(MAILDATA * pMail);
	// ȷ�������Ƿ�ɹ�
	int CheckConnection(void);
	// ʹ�ð󶨱�������ȡ�ʼ���Ϣ
	int bvGetImageInfo(ImgprcTask * pTask);
	
	int bvUpdateImgprc(ImgprcTask * pTask);
	// ʹ�ð󶨱�������XML����
//	int bvUpdateMailRecordXML(MAILDATA * pMail , DWORD dwImageNo);
	// ʹ�ð󶨱�������ͼ��
//	int bvUpdateMailRecordBR(MAILDATA * pMail, DWORD dwImageNo);
};
