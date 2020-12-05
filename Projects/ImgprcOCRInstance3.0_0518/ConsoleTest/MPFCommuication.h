#pragma once
typedef unsigned long       DWORD;
typedef unsigned char BYTE;
typedef unsigned char byte;
#define IADM_IP_LENGTH 16      //IP�ĳ���
#define IADM_PCNAME_LENGTH 20
#define	IADM_MES_MAX_LEN	513
#define IADM_SND_BUF_ROW	240 //128 2013-4-25
#define IADM_REV_BUF_ROW	240 //128 2013-4-25

#define MPF_MSG_START			0x3a    //Modbus��Ϣ֡��ʼ��: ':'
#define MPF_MSG_END1			0x0d    //Modbus��Ϣ֡������1: CR
#define MPF_MSG_END0			0x0a    //Modbus��Ϣ֡������0: LF

#define MPF_MSG_RECV_START			0
#define MPF_MSG_RECV_CONT1			1
#define MPF_MSG_RECV_CONT0			2
#define MPF_MSG_RECV_END			3

#define MPF_MSG_ADDR_IADM		0xA0	//ͼ���ϵͳ��ַ
#define MPF_MSG_ADDR_MADM		0x80
#define MPF_MSG_ADDR_OCR		0xF0	//OCR��ַ
#define MPF_MSG_ADDR_NO			2

//tx2017 ��IMAGE��ͨѶ
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

int LongToBytesBigEndian(DWORD n,DWORD H, unsigned char * bytes, int index, int len);
int BytesToLongBigEndian(DWORD & n,DWORD & H,  unsigned char * bytes, int index, int len);


class MPFCommuication
{

public:

	char m_chsIPAddress[IADM_IP_LENGTH];  //��IADMͨѶ�ļ������IP
	char m_chsPCName[IADM_PCNAME_LENGTH]; //��IADMͨѶ�ļ������
	char m_chsShortName[IADM_PCNAME_LENGTH];
	int m_chsDstPort; //��IADMͨѶ�ļ������port
	DWORD m_dwSBIDinDB;

	unsigned char m_uchValidFlag;	//��Чֵ
	unsigned char m_uchActiveFlag;	//
	DWORD m_dwTimeCount;

	// ���±���������һ���������ֽ� //
	unsigned char m_bInRevFlag;
	DWORD m_dwRevCount;
	unsigned char m_bRMesNowP;
	unsigned char m_uchsRevMesNow[IADM_MES_MAX_LEN];

	// ���ջ����� //
	DWORD m_dwRRevP;
	DWORD m_dwRProcP;
	unsigned char m_uchsRevMes[IADM_REV_BUF_ROW][IADM_MES_MAX_LEN];

	// ���ͻ����� //
	DWORD m_dwSRevP;
	DWORD m_dwSProcP;
	unsigned char m_uchsSndMes[IADM_SND_BUF_ROW][IADM_MES_MAX_LEN];
	unsigned char m_nSndMesLen[IADM_MES_MAX_LEN];

	unsigned char m_bOCRIdle;			// used for OCR and Video 

	unsigned char m_bVideoMode;			//Only used for Video
	// Error //
	DWORD m_dwUnknownMsg;    // ��Ϣ���ʹ���
	DWORD m_dwCheckSum;      // У��λ����
	DWORD m_dwBufFull;       // ��������
public:
	MPFCommuication(void);
	~MPFCommuication(void);

	int AnalyseOneComByte(unsigned char m_byteData); //���� һ�����ֽڽ�����Ϣ��
	int SendOneNetMsg(unsigned int nMsgLen, unsigned char * puchMsg);
	void ClearMachineBuf(void);
	// SOCKET MSG
	int AnalyseOneByte(unsigned char bData);
	int String2RawBytes(unsigned char* pString, int string_length, unsigned char* pBytes);//���ַ�����Ϣ��raw��Ϣ����������
	int SendOneMsgFromOCR2ImgServer(unsigned int nMsgLen, unsigned char * puchMsg);//puchMsgΪ��Ϣ�ַ�����ַ




};


