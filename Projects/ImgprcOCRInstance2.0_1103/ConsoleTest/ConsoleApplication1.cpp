// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
//#include "TmtSocket.h"K
#include<WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <string>
#include "CommonFunc.h"
#include "MPFCommuication.h"
#include <random>
using namespace std;

int server();
int get_send_msg(unsigned char *pBuffer, vector<string> imgpaths, int istopview);
int String2RawBytes(unsigned char* pString, int string_length, unsigned char* pBytes);
void ProcessOneRMes(MPFCommuication *pcomm, BYTE mbMesPos);
default_random_engine e;


int main(int argc, char* argv[])
{

	server();


	system("Pause");
}
int get_send_msg(unsigned char *pBuffer,vector<string> imgpaths,int istopview)
{
	unsigned char *pos;
	unsigned char msg_data[512*5] = { 0 };
	pos = pBuffer;
	*pos = ':';
	pos++;

	int newline = 0;
	msg_data[newline++] = 0x40;
	msg_data[newline++] = 0xC4;
	// TaskID
	//int id =  % 1000000;
	char ids[10] = { '0','1','2','3','4','5','6','7','8','9'};
	msg_data[newline++] = ids[rand()%10]; 
	msg_data[newline++] = ids[rand() % 10]; 
	msg_data[newline++] = ids[rand() % 10]; 
	msg_data[newline++] = ids[rand() % 10]; 
	msg_data[newline++] = ids[rand() % 10]; 
	msg_data[newline++] = ids[rand() % 10];
	msg_data[newline++] = (imgpaths.size()>1)?1:4; //补码类型
	msg_data[newline++] = imgpaths.size();
	string img_path_tmp;
	for (int i=0;i<imgpaths.size();i++)
	{
		img_path_tmp.append(imgpaths[i]);
		img_path_tmp.push_back(';');
	}
	unsigned int img_path_len = strlen(img_path_tmp.c_str()); //路径长度
	msg_data[newline++] = img_path_len / 256;
	msg_data[newline++] = img_path_len % 256;
	sprintf((char*)(msg_data + newline), img_path_tmp.c_str());
	newline = newline + img_path_len;
	msg_data[newline++] = istopview % 256;
	msg_data[newline++] = 10; //条码长度
	sprintf((char*)(msg_data + newline), "1234567890");
	newline = newline + 10;
	//strcpy(pos, (char*)msg_data);
	BYTE check = 0;
	for (int i = 0; i < newline; i++)
	{
		check = check + msg_data[i];
		//printf("%d ", msg_data[i]);
	}
	check = ~check;
	check++;
	//printf("\n%d\n", check);
	msg_data[newline++] = check;
	msg_data[newline] = 0;
	//printf("%s\n", msg_data);
	//printf("字符长度：%d\n", strlen((char*)msg_data));

	int data_b_length = String2RawBytes(msg_data, newline, pos);
	pos = pos + data_b_length;
	// 数据结尾
	*(pos) = 0x0D;
	*(pos + 1) = 0x0A;
	*(pos + 2) = 0;
	return strlen((char*)pBuffer);
}

int String2RawBytes(unsigned char* pString, int string_length, unsigned char* pBytes)
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

int server()
{
	WSADATA wsaData;
	int iRet = 0;
	iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRet != 0)
	{
		cout << "WSAStartup(MAKEWORD(2, 2), &wsaData) execute failed!" << endl;;
		return -1;
	}
	if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion))
	{
		WSACleanup();
		cout << "WSADATA version is not correct!" << endl;
		return -1;
	}

	//创建套接字
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		cout << "serverSocket = socket(AF_INET, SOCK_STREAM, 0) execute failed!" << endl;
		return -1;
	}

	//初始化服务器地址族变量
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr("192.168.1.30");//htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(9999);

	//绑定
	iRet = bind(serverSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	if (iRet == SOCKET_ERROR)
	{
		cout << "bind(serverSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) execute failed!" << endl;
		return -1;
	}

	//监听
	iRet = listen(serverSocket, 10);
	if (iRet == SOCKET_ERROR)
	{
		cout << "listen(serverSocket, 10) execute failed!" << endl;
		return -1;
	}
	else
	{
		cout << "Server is listening on " << addrSrv.sin_addr.S_un.S_addr << ":" << addrSrv.sin_port << "." << endl;
	}
	vector<string> imgpathvec_top;
	vector<string> imgpathvec_side;
	string imgdir_side = "F:/shared_data_original/side/*.jpg";
	string imgdir_top = "F:/shared_data_original/top/*.jpg";
	//string imgdir_top = "F:\\shared_data_original\\\top\pic2/*.jpg";
	//string imgdir_side = "F:\\shared_data_original\\top\\pic2/*.jpg";
	//string imgdir = "F:/cpte_datasets/Tailand_tag_detection_datasets/tag_obj_datasets_2/*.jpg";
	CommonFunc::getAllFilesNameInDir(imgdir_side, imgpathvec_side, true, true);
	CommonFunc::getAllFilesNameInDir(imgdir_top, imgpathvec_top, true, true);

	std::random_shuffle(imgpathvec_top.begin(), imgpathvec_top.end());
	std::random_shuffle(imgpathvec_side.begin(), imgpathvec_side.end());


	vector<string>::iterator it_top = imgpathvec_top.begin();
	if (it_top == imgpathvec_top.end())
	{
		printf("未找到顶部图片\n");
		return 0;
	}
	vector<string>::iterator it_side = imgpathvec_side.begin();
	if (it_side == imgpathvec_side.end())
	{
		printf("未找到侧面图片\n");
		return 0;
	}

	printf("找到图片数量%d\n", imgpathvec_side.size()+ imgpathvec_top.size());
	//等待连接_接收_发送

	MPFCommuication mRecOp;


	SOCKADDR_IN clientAddr;
	int len = sizeof(SOCKADDR);
	while (1)
	{
		SOCKET connSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &len);
		if (connSocket == INVALID_SOCKET)
		{
			cout << "accept(serverSocket, (SOCKADDR*)&clientAddr, &len) execute failed!" << endl;
			return -1;
		}
		else
		{
			cout << "Connection established! Waiting for messages." << endl;
		}

		int timeout = 500; //ms
		int ret = setsockopt(connSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

		int x = 0;

		while (true)
		{
			//接收消息
			int sig;
			//char recvBuf[4096];
			//int sig = recv(connSocket, recvBuf, 4096, 0);
			////没有连接时断开
			//if (sig <= 0)
			//{
			//	printf("Error: Lost connection!\n");
			//	break;
			//}
			//printf("%s\n", recvBuf);

			//发送消息
			unsigned char sendBuf[4096] = {0};
			if (it_top == imgpathvec_top.end()) it_top = imgpathvec_top.begin();
			if (it_top == imgpathvec_top.end()) it_top = imgpathvec_top.begin();
			//int msg_length = get_send_msg(sendBuf,*it);
			//it++;
			vector<string> img_sends;
			uniform_int_distribution<unsigned> u(1, 3);
			int randNum =  u(e);
			int i = 0;
			switch (randNum)
			{
			case 1: //单幅顶视图
				if (it_top == imgpathvec_top.end()) it_top = imgpathvec_top.begin();
				img_sends.push_back(*(it_top++));
				break;
			case 2: //单幅侧视图
				if (it_side == imgpathvec_side.end()) it_side = imgpathvec_side.begin();
				img_sends.push_back(*(it_side++));
				break;
			case 3: //5幅图
				for (i=0;i<5;i++) //5幅图
				{
					if (i==0)
					{
						if (it_top == imgpathvec_top.end()) it_top = imgpathvec_top.begin();
						img_sends.push_back(*(it_top++));
					}
					else
					{
						if (it_side == imgpathvec_side.end()) it_side = imgpathvec_side.begin();
						img_sends.push_back(*(it_side++));
					}
				}
				break;
			default:
				break;
			}


			int istopview = (randNum == 2) ? 0 : 1;

			int msg_length = get_send_msg(sendBuf, img_sends, istopview);


			//printf("消息长度：%d\n", msg_length);
			//printf((char*)sendBuf);
			//printf("\n");
			//sprintf_s(sendBuf, "Welcome %s", inet_ntoa(clientAddr.sin_addr));
			char str[INET_ADDRSTRLEN];
			//sprintf_s(sendBuf, "Welcome! client from %s %d!", inet_ntop(AF_INET, &clientAddr.sin_addr, str, sizeof(str)), x);
			sig = send(connSocket, (char*)sendBuf, msg_length + 1, 0);
			//int erron = GetLastError();
			//if (sig<=0 && erron == EAGAIN)
			//{
			//	printf("发送超时\n");
			//	continue;
			//}
			if (sig <= 0)
			{
				printf("Error:send Lost connection!\n");
				break;
			}
			else
			{
				printf("发送了%d幅图像\n", img_sends.size());
			}
			x++;

			char recvBuf[4096] = { 0 };
			sig = recv(connSocket, recvBuf, 4096, 0);
			//没有连接时断开
			int erron = GetLastError();
			//printf("错误代码%d\n", erron);
			if (sig <= 0 && erron == 10060)
			{
				printf("接收超时,重新发射\n");
				continue;
			}
			if (sig <= 0)
			{
				printf("Error:recv Lost connection!\n");
				break;
			}
			mRecOp.m_bInRevFlag = MPF_MSG_RECV_START;
			for (int i=0;i<sig;i++)
			{
				unsigned char uc;
				uc = recvBuf[i];
				mRecOp.AnalyseOneByte(uc);
			}
			//cout << "解析中。。。"<<endl;
			mRecOp.m_dwTimeCount = 0;


			for (DWORD j = mRecOp.m_dwRProcP; j < mRecOp.m_dwRRevP; j++)
			{
				BYTE bTemp1 = BYTE((++mRecOp.m_dwRProcP) % IADM_REV_BUF_ROW);
				ProcessOneRMes(&mRecOp, bTemp1);
			}

			cout << "解析完毕！" << endl<<endl;





			//printf("收到%d个字节\n", sig);
			//printf("%s\n", recvBuf);
			Sleep(500);
		}

		//关闭连接
		closesocket(connSocket);
	}

	system("pause");
	return 0;

}


void ProcessOneRMes(MPFCommuication *pcomm,BYTE mbMesPos)
{
	BYTE *p;
	BYTE bType;
	int nowpos = 0;
	BYTE *p_c;
	BYTE imageid[8] = { 0 };
	BYTE barcode[16] = { 0 };
	int postcode_length;
	int barcode_length;
	try
	{
		p = pcomm->m_uchsRevMes[mbMesPos];
		bType = *(p + 2); //第一位为msg length
		switch (bType)  // Message type
		{
		case MPF_MSG_OCR2IMG_RESULT:
			
			//图像id
			nowpos += 3; p_c = p + nowpos;

			memcpy(imageid, p_c, 6);
			nowpos += 6; p_c = p + nowpos;
			std::cout << "ImageID:" << imageid << endl;
			
			//ocr_id
			std::cout << "OCRID:" << unsigned int(*p_c) << endl;
			nowpos += 1; p_c = p + nowpos;

			//补码结果
			std::cout << "补码结果:" << unsigned int(*p_c) << endl;
			nowpos += 1; p_c = p + nowpos;
			
			//识别出邮编的图像编号
			std::cout << "图像序号:" << unsigned int(*p_c) << endl;
			nowpos += 1; p_c = p + nowpos;

			//条码
			barcode_length = *p_c;
			nowpos += 1; p_c = p + nowpos;

			memcpy(barcode, p_c, barcode_length);
			nowpos += barcode_length; p_c = p + nowpos;
			std::cout << "条码结果:" << barcode << endl;

			//

			postcode_length = *p_c;
			nowpos += 1; p_c = p + nowpos;
			if (postcode_length > 0)
			{
				BYTE postcode[512] = { 0 };
				memcpy(postcode, p_c, postcode_length);
				nowpos += postcode_length; p_c = p + nowpos;
				std::cout << "邮编结果:" << postcode << endl;
			}

			std::cout << "排队等侯数量：" << unsigned int(*p_c) << endl;
			//nowpos += postcode_length; p_c = p + nowpos;


			break;
		case MPF_MSG_IMG2OCR_FUNC:

			break;

		}
	}
	catch (...)
	{
		cout << "解析出现异常！" << endl;
	}
}
