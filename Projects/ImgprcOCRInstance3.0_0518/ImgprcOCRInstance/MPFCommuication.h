#pragma once
#include "logger.h"

#define IADM_IP_LENGTH 16      //IP的长度
#define IADM_PCNAME_LENGTH 20
#define	IADM_MES_MAX_LEN	5000
#define IADM_SND_BUF_ROW	240 //128 2013-4-25
#define IADM_REV_BUF_ROW	240 //128 2013-4-25

#define MPF_MSG_START			0x3a    //Modbus消息帧起始符: ':'
#define MPF_MSG_END1			0x0d    //Modbus消息帧结束符1: CR
#define MPF_MSG_END0			0x0a    //Modbus消息帧结束符0: LF

#define MPF_MSG_RECV_START			0
#define MPF_MSG_RECV_CONT1			1
#define MPF_MSG_RECV_CONT0			2
#define MPF_MSG_RECV_END			3

#define MPF_MSG_ADDR_IADM		0x40	//图像机系统地址
#define MPF_MSG_ADDR_MADM		0x80
#define MPF_MSG_ADDR_OCR		0xF0	//OCR地址
#define MPF_MSG_ADDR_NO			2

//tx2017 与IMAGE的通讯
#define MPF_MSG_IMG2OCR_REQUEST	0xC4
#define MPF_MSG_IMG2OCR_FUNC		0xC0
#define MPF_MSG_IMG2OCR_REQUEST_LEN	13  //tx
#define MPF_MSG_IMG2OCR_FUNC_LEN		24


#define MPF_MSG_OCR2IMG_RESULT	0xC5
#define MPF_MSG_OCR2IMG_RESULT_LEN 8 

#define IADM_MSG_REVBUFF_FULL		-1
#define IADM_REV_COM_MSG			1
#define IADM_MSG_CHECKERROR			-2

#define IADM_ENABLE_WAYBILL		0x01
#define IADM_ENABLE_OBR		0x02
#define IADM_ENABLE_OCR		0x04

int LongToBytesBigEndian(DWORD n,DWORD H, BYTE * bytes, int index, int len);
int BytesToLongBigEndian(DWORD & n,DWORD & H,  BYTE * bytes, int index, int len);


class MPFCommuication
{

public:

	char m_chsIPAddress[IADM_IP_LENGTH];  //与IADM通讯的计算机的IP
	char m_chsPCName[IADM_PCNAME_LENGTH];//与IADM通讯的计算机名
	char m_chsShortName[IADM_PCNAME_LENGTH];
	int m_chsDstPort; //与IADM通讯的计算机的port
	DWORD m_dwSBIDinDB;

	BYTE m_uchValidFlag;	//有效值
	BYTE m_uchActiveFlag;	//
	DWORD m_dwTimeCount;
	//存储原始缓存数据
	BYTE m_uchsRawData[IADM_MES_MAX_LEN * 2];

	// 以下变量用于逐一分析接收字节 //
	BYTE m_bInRevFlag;
	DWORD m_dwRevCount;
	BYTE m_bRMesNowP;
	BYTE m_uchsRevMesNow[IADM_MES_MAX_LEN];
	

	// 接收缓冲区 //
	DWORD m_dwRRevP;
	DWORD m_dwRProcP;
	BYTE m_uchsRevMes[IADM_REV_BUF_ROW][IADM_MES_MAX_LEN];

	// 发送缓冲区 //
	DWORD m_dwSRevP;
	DWORD m_dwSProcP;
	BYTE m_uchsSndMes[IADM_SND_BUF_ROW][IADM_MES_MAX_LEN];
	BYTE m_nSndMesLen[IADM_MES_MAX_LEN];

	BYTE m_bOCRIdle;			// used for OCR and Video 

	BYTE m_bVideoMode;			//Only used for Video
	// Error //
	DWORD m_dwUnknownMsg;    // 消息类型错误
	DWORD m_dwCheckSum;      // 校检位错误
	DWORD m_dwBufFull;       // 缓冲区满
private:
	int PushBackOneMessage(DWORD _lenth);
public:
	MPFCommuication(void);
	~MPFCommuication(void);
	BYTE* GetOneMessage();
	int AnalyseOneComByte(BYTE m_byteData); //串口 一个个字节接受消息。
	int SendOneNetMsg(unsigned int nMsgLen, BYTE * puchMsg);
	void ClearMachineBuf(void);
	// SOCKET MSG
	int AnalyseOneByte(BYTE bData);
	int String2RawBytes(unsigned char* pString, int string_length, unsigned char* pBytes);//从字符串消息到raw消息，不负责补零
	int CodeMsgFromOCR2ImgServer(unsigned int nMsgLen, unsigned char * puchMsg, unsigned char* pBuffer);//puchMsg为消息字符串地址




};


