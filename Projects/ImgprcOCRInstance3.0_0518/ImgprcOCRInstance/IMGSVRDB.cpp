#include "StdAfx.h"
#include "IMGSVRDB.h"

#define IMGSVR_FLAG_CREATED		3
#define IMGSVR_FLAG_XMLED		4
#define IMGSVR_FLAG_BRED		5
//判断是句柄有错
inline void TESTHR(HRESULT x)
{
	//CString test=((CIMRUIApp*)AfxGetApp())->IP;


	if(FAILED(x))
		_com_issue_error(x);
	return;
	
}
IMGSVRDB::IMGSVRDB(void)
{
	m_ptrConn=NULL;
	m_pSet=NULL;
	m_pSet1=NULL;

}

IMGSVRDB::~IMGSVRDB(void)
{
		try{
		if(m_pSet!=NULL)
		{
			if (m_pSet->State==adStateOpen)
			{
				m_pSet->Close();
			}
			m_pSet.Release();
		}
		if(m_pSet1!=NULL)
		{
			if (m_pSet1->State==adStateOpen)
			{
				m_pSet1->Close();
			}
			m_pSet1.Release();
		}

		if(m_ptrConn!=NULL)
		{
			if (m_ptrConn->State==adStateOpen)
			{
				m_ptrConn->Close();
			}
			m_ptrConn.Release();
		}
		::CoUninitialize();
	}catch(CException *e){
		e->Delete();
	}
}

int IMGSVRDB::InitiDB(char * sUser, char * sPassword, char * sDatabaseName, char * sDataSourceIP)
{
	
	strcpy_s(m_chsUser,sUser);
	strcpy_s(m_chsPassword,sPassword);
	strcpy_s(m_chsDBName,sDatabaseName);
	strcpy_s(m_chsDataSourceIP,sDataSourceIP);


	char sTemp[500];
	sprintf_s(sTemp,"Provider=OraOLEDB.Oracle.1;Persist Security Info=True;Data Source=%s",m_chsDBName);
	m_connstr = sTemp;

	try
	{
		if(FAILED(::CoInitialize(NULL)))
		{
			return -2;
		}
		TESTHR(m_ptrConn.CreateInstance(__uuidof(Connection)));
		m_ptrConn->Open(m_connstr,m_chsUser,m_chsPassword,adConnectUnspecified);

		TESTHR(m_pSet.CreateInstance(__uuidof(Recordset)));
		TESTHR(m_pSet1.CreateInstance(__uuidof(Recordset)));

	}
	catch(_com_error & err)
	{
		m_bstrErrDsp = err.Description();
//		TRACE("%s\n",m_bstrErrDsp);
		return -1;

	}

	return 0;
}




//#define IS_SQL_MAX_LEN 2048
//int IMGSVRDB::AddMailData2DB(MAILDATA *pMail,DWORD dwImageNo)
//{
//	char chsTemp[IS_SQL_MAX_LEN];
//	char chsTemp2[IS_SQL_MAX_LEN];
//	char chsTemp1[IS_SQL_MAX_LEN];
//	char chsTemp3[IS_SQL_MAX_LEN];
//	char chsSQLBar1[IS_SQL_MAX_LEN];
//	char chsSQLBar2[IS_SQL_MAX_LEN];
//	_variant_t vValue;
//	_variant_t RecordsAffected;
//	int bInDB = 0 ;
//
//	try
//	{
//		sprintf_s(chsTemp,"Select * FROM T_OBR_IMGSVR WHERE TRAYNO='%04d' and IMAGENO=%d ",pMail->m_dwTrayNo,dwImageNo);
//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
//		m_pSet->Open(chsTemp,m_ptrConn.GetInterfacePtr(),adOpenStatic,adLockOptimistic,adCmdText);
//		if(m_pSet->EndOfFile==FALSE)
//		{
//			//有记录
//		}
//		else
//		{
//			//无记录需要插入一条
//			sprintf_s(chsTemp,"INSERT INTO T_OBR_IMGSVR (TRAYNO,IMAGENO) VALUES('%04d', %d)",\
//				pMail->m_dwTrayNo, dwImageNo);//, pMail->m_chsImageName, pMail->m_chsFileName, pMail->m_dwImgWidth, pMail->m_dwImgHeight);
//
//			m_ptrConn->Execute(_bstr_t(chsTemp),&RecordsAffected,adCmdText);
//
//			bInDB = 0;
//		}
//			//,IMAGENAME,IMAGEPATH,WIDTH,HEIGHT
//		CString cstrTime ;	
//		cstrTime.Format(_T("to_date('%04d-%02d-%02d %02d:%02d:%02d','YYYY-MM-DD HH24:MI:SS')"),\
//			pMail->m_stLastWriteTime.wYear,pMail->m_stLastWriteTime.wMonth,pMail->m_stLastWriteTime.wDay,\
//			pMail->m_stLastWriteTime.wHour,pMail->m_stLastWriteTime.wMinute,pMail->m_stLastWriteTime.wSecond);
//		CStringToCharArray(cstrTime, chsTemp1);	
//		sprintf_s(chsTemp2,"%d,%d,%d,%d",pMail->m_dwBRLeft,pMail->m_dwBRTop,pMail->m_dwBRRight,pMail->m_dwBRBottom);
//
//
//		sprintf_s(chsTemp, "UPDATE T_OBR_IMGSVR SET CREATETIME = %s,IMAGENAME = '%s',IMAGEPATH = '%s',WIDTH=%d,HEIGHT=%d,BRWIDTH=%d,BRHEIGHT=%d,BRLOC='%s',",\
//			chsTemp1, pMail->m_chsImageName, pMail->m_chsFileName,\
//			pMail->m_dwImgWidth, pMail->m_dwImgHeight, pMail->m_dwBRImgWidth,pMail->m_dwBRImgHeight,chsTemp2);
//
//		chsSQLBar1[0]=0;
//		unsigned int i;
//		if(pMail->m_dwOBR1BarNum>0)
//		{
//			
//			for(i=0;i<pMail->m_dwOBR1BarNum;i++)
//			{
//				sprintf_s(chsTemp3,"BAR%d ='%s', BARLOC%d  = '%s',",\
//					i+1,& (pMail->m_chsBar1[i*IS_BAR_MAX_LEN]),i+1,&(pMail->m_chsPos1[i*IS_POS_MAX_LEN]));
//				strcat_s(chsSQLBar1,strlen(chsSQLBar1)+strlen(chsTemp3)+1,chsTemp3);
//			}
//			chsSQLBar1[strlen(chsSQLBar1)]=0;
//			sprintf_s(chsTemp1," BARNUM1=%d, %s",	pMail->m_dwOBR1BarNum, chsSQLBar1);
//		}
//		else
//		{
//			sprintf_s(chsTemp1,"BARNUM1=0,");
//		}
//
//		chsSQLBar2[0]=0;
//		if(pMail->m_dwOBR2BarNum>0)
//		{
//			for(i=0;i<pMail->m_dwOBR2BarNum;i++)
//			{
//				sprintf_s(chsTemp3,", BAR%d ='%s', BARLOC%d  = '%s'",\
//					i+4,& (pMail->m_chsBar2[i*IS_BAR_MAX_LEN]),i+4,&(pMail->m_chsPos2[i*IS_POS_MAX_LEN]));
//				strcat_s(chsSQLBar2,strlen(chsSQLBar1)+strlen(chsTemp3)+1,chsTemp3);
//			}
//			chsSQLBar2[strlen(chsSQLBar2)]=0;
//			sprintf_s(chsTemp2," BARNUM2=%d %s", pMail->m_dwOBR2BarNum, chsSQLBar2);
//		}
//		else
//		{
//			sprintf_s(chsTemp2,"BARNUM2=0");
//		}
//
//		sprintf_s(chsTemp3,"%s %s %s where trayno='%04d' and imageno=%d " ,\
//			chsTemp,chsTemp1,chsTemp2,pMail->m_dwTrayNo,dwImageNo);
//		m_ptrConn->Execute(_bstr_t(chsTemp3),&RecordsAffected,adCmdText);
//
//
//		return 1;
//
//	}
//	catch(_com_error & err)
//	{
//		m_bstrErrDsp = err.Description();
//		return IMGSVRDB_DBERROR;
//	}
//}
//int IMGSVRDB::DeleteRecord(DWORD dwKillTime)
//{
//	return 0;
//}

//int IMGSVRDB::GetImageInfo(MAILDATA * pMD, DWORD dwImageNo)
//{
//	_variant_t vValue;
//	_variant_t RecordsAffected;
//	int bInDB = 0 ;
//	char chsTemp[1024];
//	char chsItem[512];
//
//	unsigned int i;
//	int nbarlen=0;
//
//	CTime ctime;
//	ctime = CTime::GetCurrentTime();
//	CString cstr;
////	cstr.Format(_T("to_date('%s:00','yyyy-mm-dd hh24:mi:ss')"),ctime.Format("%Y-%m-%d %h:%m");
////	CStringToCharArray(cstr,chsItem);
//	try
//	{
//		// ORDER BY CREATETIME DESC
//		sprintf_s(chsTemp,"Select * FROM T_OBR_IMGSVR WHERE IMAGENO=%d and FLAG=%d", \
//			dwImageNo, IMGSVR_FLAG_CREATED);
//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
//		m_pSet->Open(chsTemp,m_ptrConn.GetInterfacePtr(),adOpenStatic,adLockOptimistic,adCmdText);
//		if(m_pSet->EndOfFile==FALSE)
//		{
//			vValue = m_pSet->GetCollect("trayno");
//			if(vValue.vt !=VT_NULL)
//			{
//				sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//				pMD->m_dwTrayNo = atoi(chsTemp);
//			}
//			vValue = m_pSet->GetCollect("BARNUM1");
//
//			if(vValue.vt !=VT_NULL)
//				pMD->m_dwOBR1BarNum = (DWORD)vValue.dblVal;
//			
//			if(pMD->m_dwOBR1BarNum>0)
//			{
//				for(i=0; i<IS_BAR_MAX_NUM; i++)
//				{
//					if(i<pMD->m_dwOBR1BarNum)
//					{
//						sprintf_s(chsItem,"BAR%d",i+1);
//						vValue = m_pSet->GetCollect(chsItem);
//
//						if(vValue.vt !=VT_NULL)
//						{
//						
//						sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//						nbarlen = strlen(chsTemp)+1;
//						strcpy_s(&(pMD->m_chsBar1[i*IS_BAR_MAX_LEN]),nbarlen,chsTemp);
//						}
//					}
//				}
//			}
//
//			//tx 20131210
//
//			vValue = m_pSet->GetCollect("MAILLOC");
//			if(vValue.vt!=VT_NULL)
//			{
//				sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//				strcpy_s(pMD->m_chsMailLoc, strlen(chsTemp)+1, chsTemp);
//				sscanf_s(pMD->m_chsMailLoc,"%d %d %d %d",&(pMD->m_rtMailLoc.left),&(pMD->m_rtMailLoc.top),&(pMD->m_rtMailLoc.right),&(pMD->m_rtMailLoc.bottom));
//			}
//
//			vValue = m_pSet->GetCollect("MAILTYPE");
//
//			if(vValue.vt!=VT_NULL)
//			{
//					sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//					strcpy_s(pMD->m_chsMailType, strlen(chsTemp)+1, chsTemp);
//			}
//			//
//
//			//TX 20140114
//						
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//			
//			return IMGSVRDB_RETURN_OK;
//		}
//		else
//		{
//			//TX 20140114
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//		    return IMGSVRDB_RECORDNOTINDB;
//		}	
//	}
//	catch(_com_error & err)
//	{
//		//TX 20140114
//		m_pSet->Close();
//
//		m_bstrErrDsp = err.Description();
//		return IMGSVRDB_DBERROR;
//	}
//}
//
//int IMGSVRDB::UpdateMailRecordXML(MAILDATA * pMail,DWORD dwImageNo)
//{
//	_variant_t vValue;
//	_variant_t RecordsAffected;
//	int bInDB = 0 ;
//	char chsTemp[2048]={0};
//	char chsItem[2048]={0};
//
//	char chsBar[2048]={0};
//	chsBar[0]=0;
//	CString cstrBar,cstrItem;
//	
//	
//
//	unsigned int i;
//	int nbarlen=0;
//
//	BYTE bFlag=IMGSVR_FLAG_CREATED;
//	if((pMail->m_bEnable&0x02))
//	{
//		bFlag=1;
//	}
//	try
//	{
//
//		sprintf_s(chsItem," to_date('%d-%d-%d %d:%d:%d','yyyy-mm-dd hh24:mi:ss') ",\
//			pMail->m_stInsert.wYear,pMail->m_stInsert.wMonth,pMail->m_stInsert.wDay,\
//			pMail->m_stInsert.wHour,pMail->m_stInsert.wMinute,pMail->m_stInsert.wSecond);
//
//
//		sprintf_s(chsTemp,"Select * FROM T_OBR_IMGSVR WHERE TRAYNO='%04d' and IMAGENO=%d and createtime=%d and createdate=%s", \
//			pMail->m_dwTrayNo,pMail->m_dwImageNo, pMail->m_stInsert.wMilliseconds,chsItem);
//
//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
//		m_pSet->Open(chsTemp,m_ptrConn.GetInterfacePtr(),adOpenStatic,adLockOptimistic,adCmdText);
//		if(m_pSet->EndOfFile==FALSE)
//		{
//			for(i=0; i<3;i++)
//			{
//				if(i<pMail->m_dwOBR3BarNum)
//				{
//					sprintf_s(chsBar,",BAR%d='%s',BARLOC%d='%s'",\
//						i+7, &(pMail->m_chsBar3[i*IS_BAR_MAX_LEN]),\
//						i+7, &(pMail->m_chsPos3[i*IS_POS_MAX_LEN]));
//				}
//				else
//				{
//					sprintf_s(chsBar,",BAR%d='',BARLOC%d=''",i+7,i+7);
//				}
//				cstrItem = CString(chsBar);
//				cstrBar += cstrItem;
//			}
//			CStringToCharArray(cstrBar, chsBar);
//
//			sprintf_s(chsTemp, "UPDATE T_OBR_IMGSVR set IMAGEPATH='%s',BARNUM3=%d %s WHERE TRAYNO = '%04d' and IMAGENO=%d and CREATETIME=%d and CREATEDATE=%s",\
//				pMail->m_chsNewFileName,pMail->m_dwOBR3BarNum,chsBar, pMail->m_dwTrayNo, dwImageNo,\
//				pMail->m_stInsert.wMilliseconds,chsItem);
//
//			m_ptrConn->Execute(_bstr_t(chsTemp),&RecordsAffected,adCmdText);
//
//			//TX 20140114						
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//			return IMGSVRDB_RETURN_OK;		
//		}
//		else
//		{
//			
//			//TX 20140114						
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//			return IMGSVRDB_RECORDNOTINDB;
//		}
//	}
//	catch(_com_error & err)
//	{
//		//TX 20140114
//					
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//
//		m_bstrErrDsp = err.Description();
//		return IMGSVRDB_DBERROR; 
//	}
//}

//// 更新背景删除的结果
//int IMGSVRDB::UpdateMailRecordBR(MAILDATA * pMail, DWORD dwImageNo)
//{
//	_variant_t vValue;
//	_variant_t RecordsAffected;
//	char chsTemp[2048]={0};
//	char chsItem[1024]={0};
//	try
//	{
//
//		sprintf_s(chsItem," to_date('%d-%d-%d %d:%d:%d','yyyy-mm-dd hh24:mi:ss') ",\
//			pMail->m_stInsert.wYear,pMail->m_stInsert.wMonth,pMail->m_stInsert.wDay,\
//			pMail->m_stInsert.wHour,pMail->m_stInsert.wMinute,pMail->m_stInsert.wSecond);
//
//
//		sprintf_s(chsTemp,"Select * FROM T_OBR_IMGSVR WHERE TRAYNO='%04d' and IMAGENO=%d and createtime=%d and createdate=%s", \
//			pMail->m_dwTrayNo,pMail->m_dwImageNo, pMail->m_stInsert.wMilliseconds,chsItem);
//
//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
//		m_pSet->Open(chsTemp,m_ptrConn.GetInterfacePtr(),adOpenStatic,adLockOptimistic,adCmdText);
//		if(m_pSet->EndOfFile==FALSE)
//		{
//
//			sprintf_s(chsTemp, "UPDATE T_OBR_IMGSVR set BRWIDTH =%d,BRHEIGHT=%d, BRLOC='%d %d %d %d' WHERE TRAYNO = '%04d' and IMAGENO=%d and CREATETIME=%d AND CreateDate=%s ",\
//				pMail->m_dwBRImgWidth,pMail->m_dwBRImgHeight, \
//				pMail->m_dwBRLeft, pMail->m_dwBRTop, pMail->m_dwBRRight,pMail->m_dwBRBottom,\
//				pMail->m_dwTrayNo, dwImageNo, \
//				pMail->m_stInsert.wMilliseconds,chsItem);
//
//
//			m_ptrConn->Execute(_bstr_t(chsTemp),&RecordsAffected,adCmdText);
//
//			//TX 20140114						
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//			return IMGSVRDB_RETURN_OK;		
//		}
//		else
//		{
//			//TX 20140114						
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//			return IMGSVRDB_RECORDNOTINDB;
//		}
//	}
//	catch(_com_error & err)
//	{
//		//TX 20140114
//		m_pSet->Close();
//		m_bstrErrDsp = err.Description();
//		return IMGSVRDB_DBERROR; 
//	}
//}

//// //获取邮件的OBR1的条码
//int IMGSVRDB::GetImageInfo2(MAILDATA * pMD)
//{
//	_variant_t vValue;
//	_variant_t RecordsAffected;
//	int bInDB = 0 ;
//	char chsTemp[1024]={0};
//	char chsItem[512]={0};
//
//	unsigned int i;
//	int nbarlen=0;
//
//
//	//	cstr.Format(_T("to_date('%s:00','yyyy-mm-dd hh24:mi:ss')"),ctime.Format("%Y-%m-%d %h:%m");
//	//	CStringToCharArray(cstr,chsItem);
//	try
//	{
//		
//		sprintf_s(chsItem," to_date('%d-%d-%d %d:%d:%d','yyyy-mm-dd hh24:mi:ss') ",\
//			pMD->m_stInsert.wYear,pMD->m_stInsert.wMonth,pMD->m_stInsert.wDay,\
//			pMD->m_stInsert.wHour,pMD->m_stInsert.wMinute,pMD->m_stInsert.wSecond);
//
//
//		sprintf_s(chsTemp,"Select * FROM T_OBR_IMGSVR WHERE TRAYNO='%04d' and IMAGENO=%d and FLAG=%d and createtime=%d and createdate=%s", \
//			pMD->m_dwTrayNo,pMD->m_dwImageNo, IMGSVR_FLAG_CREATED,pMD->m_stInsert.wMilliseconds,chsItem);
//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
//		m_pSet->Open(chsTemp,m_ptrConn.GetInterfacePtr(),adOpenStatic,adLockOptimistic,adCmdText);
//		if(m_pSet->EndOfFile==FALSE)
//		{
//			vValue = m_pSet->GetCollect("BARNUM1");
//
//			if(vValue.vt !=VT_NULL)
//				pMD->m_dwOBR1BarNum = (DWORD)vValue.dblVal;
//
//			if(pMD->m_dwOBR1BarNum>0)
//			{
//				for(i=0; i<IS_BAR_MAX_NUM; i++)
//				{
//					if(i<pMD->m_dwOBR1BarNum)
//					{
//						sprintf_s(chsItem,"BAR%d",i+1);
//						vValue = m_pSet->GetCollect(chsItem);
//
//						if(vValue.vt !=VT_NULL)
//						{
//
//							sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//							nbarlen = strlen(chsTemp)+1;
//							strcpy_s(&(pMD->m_chsBar1[i*IS_BAR_MAX_LEN]),nbarlen,chsTemp);
//						}
//					}
//				}
//			}
//
//			//tx 20131210
//
//			vValue = m_pSet->GetCollect("MAILLOC");
//			if(vValue.vt!=VT_NULL)
//			{
//				sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//				strcpy_s(pMD->m_chsMailLoc, strlen(chsTemp)+1, chsTemp);
//				sscanf_s(pMD->m_chsMailLoc,"%d %d %d %d",&(pMD->m_rtMailLoc.left),&(pMD->m_rtMailLoc.top),&(pMD->m_rtMailLoc.right),&(pMD->m_rtMailLoc.bottom));
//			}
//
//			vValue = m_pSet->GetCollect("MAILTYPE");
//
//			if(vValue.vt!=VT_NULL)
//			{
//				sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//				strcpy_s(pMD->m_chsMailType, strlen(chsTemp)+1, chsTemp);
//			}
//			//
//
//			//TX 20140114
//
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//
//			return IMGSVRDB_RETURN_OK;
//		}
//		else
//		{
//			//TX 20140114
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//			return IMGSVRDB_RECORDNOTINDB;
//		}	
//	}
//	catch(_com_error & err)
//	{
//		//TX 20140114
//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
//
//		m_bstrErrDsp = err.Description();
//		return IMGSVRDB_DBERROR;
//	}
//}


// 确认连接是否成功
int IMGSVRDB::CheckConnection(void)
{
	variant_t RecordsAffected;
	try
	{
		if(m_ptrConn)
		{
			if(m_ptrConn->State==adStateClosed)
			{
				m_ptrConn->Open(m_connstr,m_chsUser,m_chsPassword,adConnectUnspecified);
				return IMGSVRDB_RECONECTED;
			}
			else
			{
				m_ptrConn->Execute(_bstr_t("select * from t_sys_parainfo t where rownum=1"),&RecordsAffected,adCmdText);
				return IMGSVRDB_RETURN_OK;
			}

		}
		else
		{
			return IMGSVRDB_CONN_NOTCREATED;
		}

	}
	catch (_com_error & err)
	{
		if(m_ptrConn)
		{
			if(m_ptrConn->State==adStateOpen)
			{
				m_ptrConn->Close();
			}
		}
		m_bstrErrDsp = err.Description();
		TRACE("%s\n",m_bstrErrDsp);
		return IMGSVRDB_CONN_CLOSED;			
	}

}

// 使用绑定变量来获取邮件信息
int IMGSVRDB::bvGetImageInfo(ImgprcTask * pTask)
{
//	_variant_t vValue,v_time;
//	_variant_t RecordsAffected;
//	int bInDB = 0 ;
//	char chsTemp[1024]={0};
//	char chsItem[512]={0};
//
//	unsigned int i;
//	int nbarlen=0;
//
//	_CommandPtr pCmd=NULL;
//
//
//	COleDateTime dt;
//	//dt.SetDateTime(pMD->m_stInsert.wYear, pMD->m_stInsert.wMonth, pMD->m_stInsert.wDay, \
//	//	pMD->m_stInsert.wHour, pMD->m_stInsert.wMinute, pMD->m_stInsert.wSecond);
//	//v_time.vt = VT_DATE;
//	//v_time.date = dt;
//	char chsTrayno[10]={0};
//
//
//	//	cstr.Format(_T("to_date('%s:00','yyyy-mm-dd hh24:mi:ss')"),ctime.Format("%Y-%m-%d %h:%m");
//	//	CStringToCharArray(cstr,chsItem);
//	try
//	{
//		
//		TESTHR(pCmd.CreateInstance(__uuidof(Command)));		//TX20131009 不考虑类型
//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
////		sprintf_s(chsTrayno,"%04d",pMD->m_dwTrayNo);
//		pCmd->CommandText="select * from t_ovcs_task_detail t  where IMAGEID=? ";
//
//		pCmd->Parameters->Append(pCmd->CreateParameter("IMAGEID", adNumeric, adParamInput, 14, pTask->m_dwHImageID*4294967296+pTask->m_dwLImageID));
//		pCmd->PutActiveConnection(_variant_t((IDispatch*)this->m_ptrConn));
//
//		m_pSet = pCmd->Execute(NULL,NULL,adCmdText);
//		pCmd.Release();
//
//		if(m_pSet->EndOfFile==FALSE)
//		{
//
//			vValue  = m_pSet->GetCollect("BACKUPPATH");
//
//
//
//			if(vValue.vt !=VT_NULL)
//				sprintf_s(pTask->m_chsFileName,(char *)(_bstr_t)vValue);
//
//			vValue = m_pSet->GetCollect("BARNUM");
//
//			if(vValue.vt !=VT_NULL)
//				pTask->m_iPreOBRBarNum = vValue.intVal;
//
//			
//
//			if(pTask->m_iPreOBRBarNum>0)
//			{
//				for(i=0; i<IS_BAR_MAX_NUM; i++)
//				{
//					if(i<pTask->m_iPreOBRBarNum)
//					{
//						sprintf_s(chsItem,"BARCODE%d",i+1);
//						vValue = m_pSet->GetCollect(chsItem);
//
//						if(vValue.vt !=VT_NULL)
//						{
//
//							sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//							nbarlen = strlen(chsTemp)+1;
//							strcpy_s(&(pTask->m_chsPreBars[i*IS_BAR_MAX_LEN]),nbarlen,chsTemp);
//						}
//
//						sprintf_s(chsItem,"BARLOC%d",i+1);
//						vValue = m_pSet->GetCollect(chsItem);
//
//						if(vValue.vt !=VT_NULL)
//						{
//
//							sprintf_s(chsTemp,(char *)(_bstr_t)vValue);
//							nbarlen = strlen(chsTemp)+1;
//							strcpy_s(&(pTask->m_chsPreBarLocs[i*IS_BAR_MAX_LEN]),nbarlen,chsTemp);
//						}
//
//					}
//				}
//			}
//
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//
//			return IMGSVRDB_RETURN_OK;
//		}
//		else
//		{
//			//TX 20140114
//			if (m_pSet->State==adStateOpen)
//			{
//				m_pSet->Close();
//			}
//			return IMGSVRDB_RECORDNOTINDB;
//		}	
//	}
//	catch(_com_error & err)
//	{
//		//TX 20140114
//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
//
//		m_bstrErrDsp = err.Description();
//		return IMGSVRDB_DBERROR;
//	}
	return 0;
}




int IMGSVRDB::bvUpdateImgprc(ImgprcTask * pTask)
{


	variant_t vValue,v_time;
	_variant_t RecordsAffected;
	int bInDB = 0 ;
	char chsTemp[256]={0};
	char chsItem[512]={0};

	unsigned int i;
	int nbarlen=0;

	_CommandPtr pCmd=NULL;
	_CommandPtr pCmdRead=NULL;
	_RecordsetPtr pSet;
	try
	{
		
		TESTHR(pCmd.CreateInstance(__uuidof(Command)));		//TX20131009 不考虑类型
		
		sprintf(chsTemp,"%d%d",pTask->nXrayReturn,pTask->nXrayType);
		pCmd->CommandText="UPDATE t_IMGPRC_TASK t SET HASWAYBILL=?, DONEPATH=? , PROCESSINGTIME=?, OCRRESULT=?, updatetime = sysdate where IMAGEID=? ";
		
		pCmd->Parameters->Append(pCmd->CreateParameter("HASWAYBILL", adChar, adParamInput, 1, pTask->m_bHasWayBill));
		pCmd->Parameters->Append(pCmd->CreateParameter("DONEPATH", adVarChar, adParamInput, 512, pTask->m_chsDonePath));
		pCmd->Parameters->Append(pCmd->CreateParameter("PROCESSINGTIME", adInteger, adParamInput, 4, pTask->m_iPrcTime));
		pCmd->Parameters->Append(pCmd->CreateParameter("OCRRESULT", adVarChar, adParamInput, 256, chsTemp));

		pCmd->Parameters->Append(pCmd->CreateParameter("IMAGEID", adNumeric, adParamInput, 14, pTask->m_dwHImageID*4294967296 +pTask->m_dwLImageID));
		pCmd->PutActiveConnection(_variant_t((IDispatch*)this->m_ptrConn));
		pCmd->Execute(NULL,NULL,adCmdText);


		pCmd.Release();
		return IMGSVRDB_RETURN_OK;
	}
	catch(_com_error & err)
	{


		m_bstrErrDsp = err.Description();
		return IMGSVRDB_DBERROR;
	}
}


/*
// 使用绑定变量更新XML内容
int IMGSVRDB::bvUpdateMailRecordXML(MAILDATA * pMail , DWORD dwImageNo)
{
	_variant_t vValue, v_time;

	_variant_t RecordsAffected;
	int bInDB = 0 ;
	char chsTemp[2048]={0};
	char chsItem[2048]={0};

	char chsBar[2048]={0};
	chsBar[0]=0;
	CString cstrBar,cstrItem;
	
		_CommandPtr pCmd=NULL;


	COleDateTime dt;
	dt.SetDateTime(pMail->m_stInsert.wYear, pMail->m_stInsert.wMonth, pMail->m_stInsert.wDay, \
		pMail->m_stInsert.wHour, pMail->m_stInsert.wMinute, pMail->m_stInsert.wSecond);
	v_time.vt = VT_DATE;
	v_time.date = dt;
	char chsTrayno[10]={0};
	

	unsigned int i;
	int nbarlen=0;

	BYTE bFlag=IMGSVR_FLAG_CREATED;
	if((pMail->m_bEnable&0x02))
	{
		bFlag=1;
	}
	try
	{

	//	sprintf_s(chsItem," to_date('%d-%d-%d %d:%d:%d','yyyy-mm-dd hh24:mi:ss') ",\
	//		pMail->m_stInsert.wYear,pMail->m_stInsert.wMonth,pMail->m_stInsert.wDay,\
	//		pMail->m_stInsert.wHour,pMail->m_stInsert.wMinute,pMail->m_stInsert.wSecond);


//		sprintf_s(chsTemp,"Select * FROM T_OBR_IMGSVR WHERE TRAYNO='%04d' and IMAGENO=%d and createtime=%d and createdate=%s", \
//			pMail->m_dwTrayNo,pMail->m_dwImageNo, pMail->m_stInsert.wMilliseconds,chsItem);

//		if (m_pSet->State==adStateOpen)
//		{
//			m_pSet->Close();
//		}
//		m_pSet->Open(chsTemp,m_ptrConn.GetInterfacePtr(),adOpenStatic,adLockOptimistic,adCmdText);
//		if(m_pSet->EndOfFile==FALSE)
//		{

		TESTHR(pCmd.CreateInstance(__uuidof(Command)));		//TX20131009 不考虑类型

		sprintf_s(chsTrayno,"%04d",pMail->m_dwTrayNo);
		pCmd->CommandText="UPDATE T_OBR_IMGSVR set IMAGEPATH=?,BARNUM3=?, bar7=?,BARLOC7=?,bar8=?,BARLOC8=?,bar9=?,BARLOC9=? WHERE TRAYNO = ? and IMAGENO=? and CREATETIME=? and CREATEDATE=?";
		pCmd->Parameters->Append(pCmd->CreateParameter("IMAGPATH",adVarChar,adParamInput, 200,pMail->m_chsNewFileName));
		pCmd->Parameters->Append(pCmd->CreateParameter("BARNUM3",adInteger,adParamInput,4,pMail->m_dwOBR3BarNum));

		pCmd->Parameters->Append(pCmd->CreateParameter("bar7",adVarChar,adParamInput,50,&(pMail->m_chsBar3[0])));
		pCmd->Parameters->Append(pCmd->CreateParameter("BARLOC7",adVarChar,adParamInput,50,&(pMail->m_chsPos3[0])));

		pCmd->Parameters->Append(pCmd->CreateParameter("bar8",adVarChar,adParamInput,50,&(pMail->m_chsBar3[IS_BAR_MAX_LEN])));
		pCmd->Parameters->Append(pCmd->CreateParameter("BARLOC8",adVarChar,adParamInput,50, &(pMail->m_chsPos3[IS_POS_MAX_LEN])));

		pCmd->Parameters->Append(pCmd->CreateParameter("bar9",adVarChar,adParamInput,50,&(pMail->m_chsBar3[IS_BAR_MAX_LEN*2])));
		pCmd->Parameters->Append(pCmd->CreateParameter("BARLOC9",adVarChar,adParamInput,50,&(pMail->m_chsPos3[IS_POS_MAX_LEN*2])));


		pCmd->Parameters->Append(pCmd->CreateParameter("TRAYNO", adVarChar, adParamInput, 4,chsTrayno));
		pCmd->Parameters->Append(pCmd->CreateParameter("IMAGENO", adInteger, adParamInput, 4, pMail->m_dwImageNo));
		pCmd->Parameters->Append(pCmd->CreateParameter("CREATETIME", adInteger, adParamInput, 4, pMail->m_stInsert.wMilliseconds));
		pCmd->Parameters->Append(pCmd->CreateParameter("CREATEDATE", adDBTimeStamp, adParamInput, sizeof(v_time),v_time));
		pCmd->PutActiveConnection(_variant_t((IDispatch*)this->m_ptrConn));

		pCmd->Execute(NULL,NULL,adCmdText);
		pCmd.Release();
		return IMGSVRDB_RETURN_OK;
	}
	catch(_com_error & err)
	{
		m_bstrErrDsp = err.Description();
		return IMGSVRDB_DBERROR; 
	}
}
*/
/*
int IMGSVRDB::bvUpdateMailRecordBR(MAILDATA * pMail, DWORD dwImageNo)
{

	_variant_t vValue, v_time;

	_variant_t RecordsAffected;
	int bInDB = 0 ;
	char chsItem[2048]={0};

	_CommandPtr pCmd=NULL;


	COleDateTime dt;
	dt.SetDateTime(pMail->m_stInsert.wYear, pMail->m_stInsert.wMonth, pMail->m_stInsert.wDay, \
		pMail->m_stInsert.wHour, pMail->m_stInsert.wMinute, pMail->m_stInsert.wSecond);
	v_time.vt = VT_DATE;
	v_time.date = dt;
	char chsTrayno[10]={0};
	try
	{

		TESTHR(pCmd.CreateInstance(__uuidof(Command)));		//TX20131009 不考虑类型
		sprintf_s(chsTrayno,"%04d",pMail->m_dwTrayNo);
		sprintf_s(chsItem,"%d %d %d %d",pMail->m_dwBRLeft, pMail->m_dwBRTop, pMail->m_dwBRRight,pMail->m_dwBRBottom);

		pCmd->CommandText="UPDATE T_OBR_IMGSVR set BRWIDTH =?,BRHEIGHT=?, BRLOC=? WHERE TRAYNO = ? and IMAGENO=? and CREATETIME=? and CREATEDATE=?";
		pCmd->Parameters->Append(pCmd->CreateParameter("BRWIDTH",adInteger,adParamInput,4,pMail->m_dwBRImgHeight));
		pCmd->Parameters->Append(pCmd->CreateParameter("BRHEIGHT",adInteger,adParamInput,4,pMail->m_dwBRImgWidth));
		pCmd->Parameters->Append(pCmd->CreateParameter("BRLOC",adVarChar,adParamInput,50,chsItem));

		pCmd->Parameters->Append(pCmd->CreateParameter("TRAYNO", adVarChar, adParamInput, 4,chsTrayno));
		pCmd->Parameters->Append(pCmd->CreateParameter("IMAGENO", adInteger, adParamInput, 4, pMail->m_dwImageNo));
		pCmd->Parameters->Append(pCmd->CreateParameter("CREATETIME", adInteger, adParamInput, 4, pMail->m_stInsert.wMilliseconds));
		pCmd->Parameters->Append(pCmd->CreateParameter("CREATEDATE", adDBTimeStamp, adParamInput, sizeof(v_time),v_time));
		pCmd->PutActiveConnection(_variant_t((IDispatch*)this->m_ptrConn));

		pCmd->Execute(NULL,NULL,adCmdText);
		pCmd.Release();
		return IMGSVRDB_RETURN_OK;
	}
	catch(_com_error & err)
	{
		//TX 20140114
		m_pSet->Close();
		m_bstrErrDsp = err.Description();
		return IMGSVRDB_DBERROR; 
	}
}

*/