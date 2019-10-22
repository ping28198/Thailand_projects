//#include "StdAfx.h"
#include "MPFCommuication.h"
#include <string.h> 
#include "math.h"
#include <algorithm>

int  MPFCommuication::AnalyseOneComByte(unsigned char byteData)
{
	unsigned int nMsgLen=0;
	unsigned int nBarNum=0;
	unsigned int i=0;
	unsigned char check=0;
	DWORD dwTemp1;
	int reVel=0;
	switch(this->m_bInRevFlag)
	{
	case MPF_MSG_RECV_START:
		if(byteData==MPF_MSG_START)
		{
			m_bInRevFlag = MPF_MSG_RECV_CONT1;
			m_dwRevCount = 0;

		}
		break;
	case MPF_MSG_RECV_CONT1:
		if ((byteData >= '0') && (byteData <= '9'))
		{
			byteData -= 0x30;
			m_uchsRevMesNow[m_dwRevCount] = byteData << 4;			
			m_bInRevFlag = MPF_MSG_RECV_CONT0;
		}
		else if ((byteData >= 'A') && (byteData <= 'F'))
		{
			byteData -= 0x37;
			m_uchsRevMesNow[m_dwRevCount] = byteData << 4;
			m_bInRevFlag = MPF_MSG_RECV_CONT0;
		}
		else if (byteData == MPF_MSG_END1)     //接收到结束符1
		{
			m_bInRevFlag = MPF_MSG_RECV_END;
		}
		else
		{
			m_bInRevFlag = MPF_MSG_RECV_START;
		}
		break;
	case MPF_MSG_RECV_CONT0:
		if ((byteData >= '0') && (byteData <= '9'))
		{
			byteData -= 0x30;
			m_uchsRevMesNow[m_dwRevCount++] |= byteData;
			m_bInRevFlag  = MPF_MSG_RECV_CONT1;
		}
		else if ((byteData >= 'A') && (byteData <= 'F'))
		{
			byteData -= 0x37;
			m_uchsRevMesNow[m_dwRevCount++] |= byteData;
			m_bInRevFlag  = MPF_MSG_RECV_CONT1;
		}
		else
		{
			m_bInRevFlag =  MPF_MSG_RECV_START;
		}
		break;

	case MPF_MSG_RECV_END:
		if (byteData == MPF_MSG_END0)                              //结束符0正确
		{
			nMsgLen = 0;
			if (m_uchsRevMesNow[0] == MPF_MSG_ADDR_IADM)     //地址有效
			{			
				if(m_uchsRevMesNow[1]==MPF_MSG_IMG2OCR_REQUEST)
				{	
					nMsgLen = m_dwRevCount-1;
				}
				else if(m_uchsRevMesNow[1]==MPF_MSG_IMG2OCR_FUNC)
				{
					nMsgLen = m_dwRevCount - 1;
				}


				if(nMsgLen>0)
				{
					check = 0;
					for (i=0; i<nMsgLen; i++)
					{
						check += m_uchsRevMesNow[i];
					}
					check = ~check;
					check++;
					if (check == m_uchsRevMesNow[nMsgLen])                 //LRC校验正确
					{
						if( (m_dwRRevP-m_dwRProcP)>=(IADM_REV_BUF_ROW-1))
						{
							reVel = IADM_MSG_REVBUFF_FULL;
						}
						else
						{	
							dwTemp1=(m_dwRRevP+1)%IADM_REV_BUF_ROW;
							m_uchsRevMes[dwTemp1][0]=nMsgLen;
							for (i=0;i<m_dwRevCount;i++) 
							{
								m_uchsRevMes[dwTemp1][i+1]=m_uchsRevMesNow[i];
							}
							m_dwRRevP++;
							reVel = IADM_REV_COM_MSG;
						}
					}
					else
					{
						reVel = IADM_MSG_CHECKERROR;
					}
				}
			}
		}
		m_bInRevFlag  = MPF_MSG_RECV_START;
		break;

	default:
		m_bInRevFlag  = MPF_MSG_RECV_START;
		break;
	}

	return reVel;
}

// SOCKET MSG




int MPFCommuication::AnalyseOneByte(unsigned char byteData)
{
	unsigned int nMsgLen=0;
	unsigned int nBarNum=0;
	unsigned int i=0;
	unsigned char check=0;
	DWORD dwTemp1;
	int reVel=0;

	switch(this->m_bInRevFlag)
	{
	case MPF_MSG_RECV_START:
		if(byteData==MPF_MSG_START)
		{
			m_bInRevFlag = MPF_MSG_RECV_CONT1;
			m_dwRevCount = 0;
		}
		break;
	case MPF_MSG_RECV_CONT1:
		if ((byteData >= '0') && (byteData <= '9'))
		{
			byteData -= 0x30;
			m_uchsRevMesNow[m_dwRevCount] = byteData << 4;			
			m_bInRevFlag = MPF_MSG_RECV_CONT0;
		}
		else if ((byteData >= 'A') && (byteData <= 'F'))
		{
			byteData -= 0x37;
			m_uchsRevMesNow[m_dwRevCount] = byteData << 4;
			m_bInRevFlag = MPF_MSG_RECV_CONT0;
		}
		else if (byteData == MPF_MSG_END1)     //接收到结束符1
		{
			m_bInRevFlag = MPF_MSG_RECV_END;
		}
		else
		{
			m_bInRevFlag = MPF_MSG_RECV_START;
		}
		break;
	case MPF_MSG_RECV_CONT0:
		if ((byteData >= '0') && (byteData <= '9'))
		{
			byteData -= 0x30;
			m_uchsRevMesNow[m_dwRevCount++] |= byteData;
			m_bInRevFlag  = MPF_MSG_RECV_CONT1;
		}
		else if ((byteData >= 'A') && (byteData <= 'F'))
		{
			byteData -= 0x37;
			m_uchsRevMesNow[m_dwRevCount++] |= byteData;
			m_bInRevFlag  = MPF_MSG_RECV_CONT1;
		}
		else
		{
			m_bInRevFlag =  MPF_MSG_RECV_START;
		}
		break;

	case MPF_MSG_RECV_END:

		if (byteData ==  MPF_MSG_END0)                              //结束符0正确
		{

			nMsgLen = 0;
			if (m_uchsRevMesNow[0] == MPF_MSG_ADDR_IADM	|| m_uchsRevMesNow[0] == MPF_MSG_ADDR_OCR )     //地址有效
			{
				if(m_uchsRevMesNow[1] == MPF_MSG_OCR2IMG_RESULT)
				{	
					nMsgLen = m_dwRevCount-1;
				}//tx 20140529
				else if(m_uchsRevMesNow[1] == MPF_MSG_IMG2OCR_FUNC)	//madm 设置
				{
					nMsgLen = m_dwRevCount-1;
				}
				if(nMsgLen>0)
				{
					check = 0;
					for (i=0; i<nMsgLen; i++)
					{
						check += m_uchsRevMesNow[i];
					}
					check = ~check;
					check++;
					if (check == m_uchsRevMesNow[nMsgLen])//LRC校验正确
					{
						if( (m_dwRRevP-m_dwRProcP)>=(IADM_REV_BUF_ROW-1))
						{
							reVel = IADM_MSG_REVBUFF_FULL;
						}
						else
						{

							dwTemp1=(m_dwRRevP+1)%IADM_REV_BUF_ROW;
							m_uchsRevMes[dwTemp1][0]=nMsgLen;
							for (i=0;i<m_dwRevCount;i++) 
							{
								m_uchsRevMes[dwTemp1][i+1]=m_uchsRevMesNow[i];
							}
							m_dwRRevP++;
							reVel = IADM_REV_COM_MSG;

						}
					}
				}
			}
		}
		m_bInRevFlag  = MPF_MSG_RECV_START;
		break;

	default:
		m_bInRevFlag  = MPF_MSG_RECV_START;
		break;
	}

	return reVel;

}

int MPFCommuication::String2RawBytes(unsigned char* pString, int string_length, unsigned char* pBytes)
{
	int i_S = 0;
	int i_B = 0;
	char hexes[17] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F','\0' };

	unsigned char a;
	int m_count = 0;
	unsigned char *pos = pString;
	unsigned char *bos = pBytes;

	for (int i = 0; i < string_length; i++)
	{
		a = *pos;
		unsigned int i_l = a >> 4;
		unsigned int i_r = a & 0x0F;
		*bos = hexes[i_l];
		*(bos + 1) = hexes[i_r];
		pos++;
		bos = bos + 2;
		m_count = m_count + 2;
	}
	
	return m_count;
}


//成功1 失败0:接收无效 -1 缓冲区溢出 -2消息长度溢出
//在发送缓冲区中增加一个发送消息
int MPFCommuication::SendOneMsgFromOCR2ImgServer(unsigned int nMsgLen, unsigned char * puchMsg)
{
	unsigned int newLen = 0;
	unsigned char byte = 0;
	unsigned char check = 0;
	unsigned char uchNew[IADM_MES_MAX_LEN];
	memset(uchNew, 0, IADM_MES_MAX_LEN);
	uchNew[newLen++] = MPF_MSG_START;
	
	//定义消息头
	unsigned char msgHeader[16] = { 0 };
	msgHeader[0] = 0xA0;
	msgHeader[1] = MPF_MSG_OCR2IMG_RESULT;
	int headerLength = String2RawBytes(msgHeader,2, uchNew + newLen);
	newLen = newLen + headerLength;

	//生成消息内容
	if (nMsgLen>252)
	{
		return -2;
	}
	int msg_length = String2RawBytes(puchMsg, nMsgLen, uchNew + newLen);
	newLen = newLen + msg_length;

	//生成校验码
	check = check + msgHeader[0];
	check = check + msgHeader[1];
	for (int i=0;i<nMsgLen;i++)
	{
		check = check + *(puchMsg + i);
	}
	check = ~check; //取反
	check++;
	unsigned char check_byte[2] = { 0 };
	check_byte[0] = check;
	int check_length = String2RawBytes(check_byte, 1, uchNew + newLen);
	newLen = newLen + check_length;

	//定义消息结尾
	uchNew[newLen++] = MPF_MSG_END1;
	uchNew[newLen++] = MPF_MSG_END0;
	uchNew[newLen] = 0;

	//将消息推入缓冲区
	DWORD dwTemp1;
	int bFlag = 0;
	if (this->m_uchValidFlag == 0)
	{
		bFlag = 0;// 本机器无效
	}
	else
	{
		if ((m_dwSRevP - m_dwSProcP) >= (IADM_SND_BUF_ROW - 1))
		{
			bFlag = -1; //缓冲区溢出
		}
		else
		{
			// Fill message into send buffer
			dwTemp1 = (m_dwSRevP + 1) % IADM_SND_BUF_ROW;
			this->m_nSndMesLen[dwTemp1] = newLen;
			int i = 0;
			for (i = 0; i < newLen; i++)
			{
				m_uchsSndMes[dwTemp1][i] = uchNew[i];
			}
			m_uchsSndMes[dwTemp1][i] = '\0';
			m_dwSRevP++;
			bFlag = 1;
		}
	}
	return bFlag;

}

MPFCommuication::MPFCommuication()
{
	memset(this,0,sizeof(MPFCommuication));
}

MPFCommuication::~MPFCommuication()
{
}
//成功1 失败0:接收无效 -1 缓冲区溢出 -2消息长度溢出
//在发送缓冲区中增加一个发送消息
int MPFCommuication::SendOneNetMsg(unsigned int nMsgLen, unsigned char * puchMsg)
{
	unsigned int newLen  = 0;
	unsigned char byte=0;
	unsigned char check = 0;
	unsigned char uchNew[IADM_MES_MAX_LEN];
	memset(uchNew,0,IADM_MES_MAX_LEN);
	uchNew[newLen++] = MPF_MSG_START;
	unsigned int i;
	for(i=1;i<nMsgLen;i++)
	{
		check +=puchMsg[i];
		byte=puchMsg[i]>>4;
		if(byte>=0 && byte<=9)
		{
			uchNew[newLen++] = byte+'0';
		}
		else if(byte>=0x0a && byte<=0x0f)
		{
			uchNew[newLen++] = byte-0x0a+'A';
		}
		else
		{
			uchNew[newLen++] = 0;
		}
		byte = puchMsg[i] &0x0f;
		if(byte>=0 && byte<=9)
		{
			uchNew[newLen++] = byte+'0';
		}
		else if(byte>=0x0a && byte<=0x0f)
		{
			uchNew[newLen++] = byte-0x0a+'A';
		}
		else///
		{
			uchNew[newLen++] = 0;
		}
		if(newLen>=IADM_MES_MAX_LEN)
		{
			return -2;//tx 20140517 长度非法
		}
	}

	check = ~check;
	//check++;
	byte=check>>4;
	if(byte>=0 && byte<=9)
	{
		uchNew[newLen++] = byte+'0';
	}
	else if(byte>=0x0a && byte<=0x0f)
	{
		uchNew[newLen++] = byte-0x0a+'A';
	}
	else
	{
		uchNew[newLen++] = 0;
	}
	byte = check & 0x0f;
	if(byte>=0 && byte<=9)
	{
		uchNew[newLen++] = byte+'0';
	}
	else if(byte>=0x0a && byte<=0x0f)
	{
		uchNew[newLen++] = byte-0x0a+'A';
	}
	else
	{
		uchNew[newLen++] = 0;
	}
	uchNew[newLen++] = MPF_MSG_END1;
	uchNew[newLen++] = MPF_MSG_END0;
	uchNew[newLen] = 0;
	DWORD dwTemp1;
	int bFlag = 0;
	if(this->m_uchValidFlag ==0)
	{
		bFlag=0;// 本机器无效
	}
	else
	{
		if ((m_dwSRevP-m_dwSProcP)>=(IADM_SND_BUF_ROW-1)) 
		{
			bFlag=-1; //缓冲区溢出
		}
		else
		{
			// Fill message into send buffer
			dwTemp1=(m_dwSRevP+1)%IADM_SND_BUF_ROW;
			this->m_nSndMesLen[dwTemp1] = newLen;

			for (i=0;i<newLen;i++)
			{
				m_uchsSndMes[dwTemp1][i]=uchNew[i];
			}
			m_uchsSndMes[dwTemp1][i]='\0';
			m_dwSRevP++;
			bFlag = 1;
		}	
	}
	return bFlag;

}
void MPFCommuication::ClearMachineBuf(void)
{

	this->m_uchValidFlag=0;
	this->m_bInRevFlag=0;
	this->m_dwRRevP=0;
	this->m_dwRProcP=0;	
	this->m_dwSRevP=0;
	this->m_dwSProcP=0;
	this->m_bOCRIdle=1;
	this->m_dwUnknownMsg=0;
	this->m_dwCheckSum=0;
	this->m_dwBufFull=0;

}


int LongToBytesBigEndian(DWORD Low, DWORD Hig , unsigned char * bytes, int index, int len)
{

            if (8 < len)
            {
                return (-1);
            }
			int i=0;
			if(len<=4)
			{
				for(i=0;i<len;i++)
				{
					bytes[index + i] = (byte)( (Low >> ((len - 1 - i) * 8)) & 0xFF);
				}
			}
			else
			{
			int Length = len-4;
				for (i=0; i < Length; i++)
				{
					bytes[index + i] = (byte)( (Hig >> ((Length - 1 - i) * 8)) & 0xFF);
				}
				for(i=0;i<4;i++)
				{
					bytes[index+Length+ i] = (byte)( (Low >> ((4- 1 - i) * 8)) & 0xFF);
				}
			}


            return 0;
}

int BytesToLongBigEndian(DWORD & nLow,DWORD & nHig, unsigned char * bytes, int index, int len)
{
            if (8 < len)
            {
                nLow=0; nHig=0;
                return (-1);
            }

            nLow = 0;
			nHig=0;
			int Length = std::max(0,len-4);
			int i=0;
            for ( i = 0; i < Length; ++i)
            {
                nHig = (nHig << 8) | bytes[index + i];
            }
			for(;i<len;i++)
			{
				nLow = (nLow << 8) | bytes[index + i];
			}

            return 0;
}