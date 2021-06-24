#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <queue>
#include <condition_variable>
#include <thread>
#include "EasySocket.h"
#include "SocketErrors.h"
#include <atomic> 

#ifndef uchar
typedef unsigned char uchar;
#endif // !uchar

typedef unsigned int client_id;

#define MSG_BIG_ENDIAN 1 //1��ˣ�0 С��

//#define SF_EXPRESS
#define CHINA_POST_PROTO 1
#define SF_EXPRESS_PROTO 0

#define DEBUG_NETWORK

//�첽��Ϣ������ȡ


class NetworkServer
{
// �ⲿ�߳�ʹ��
public:
	NetworkServer(uint16_t in_port);
	NetworkServer(uint16_t in_port,std::string start_c= ":",std::string end_c= "\x0D\x0A",int protocol=0); //0：顺丰，1：邮政
	~NetworkServer();
	int recv_one_message(std::string &dst,client_id &cid);
	int recv_one_message_no_block(std::string &dst, client_id &cid);
	int message_num_to_recv();

	int send_message_to_one(std::string &msg,client_id cid);
	int send_message_to_all(std::string msg);


	static std::string decode_data_hex(std::string &src_data);
	static std::string encode_data_hex(std::string &src_data);
	static bool check_msg_validation_china_post(std::string &src_data);
	static unsigned char generate_msg_validation_code_china_post(const std::string &src_data);

	//�̺߳���
	static void main_process(void *parent, uint16_t in_port);
// �ڲ������߳�ʹ��
public:
	int _push_one_message(std::string msg, client_id cid);
	int _acquire_one_message(std::string &msg, client_id cid);
	int _acquire_one_message_no_block(std::string &msg, client_id cid);
	int _register_one_client();
	int _close_one_client(client_id cid);
	int _start_run();
	int _is_big_endien();
//��Ա����
public:
	int m_protocol = 0;
	//Log::Logger *logger = nullptr;
	int m_msg_num_tosend = 0;
	std::string m_start_c;
	std::string m_end_c;
	bool is_running = true;
	bool m_is_big_endien = false;

	SocketLib::ListeningSocket *m_pLSock = nullptr;
	std::vector<SocketLib::DataSocket> m_DSocks;
	std::vector<std::thread *> m_client_threads;
	std::thread *pMainwoker;
protected:
	int m_in_port;
	const int max_message_num=512;
	std::vector<std::pair<std::string,client_id>> m_messages_send;
	std::vector<std::pair<std::string, client_id>> m_messages_recv;
	std::vector<bool> m_clients_iswork;
	std::vector<unsigned int> m_flag_message_send;

	std::mutex m_mutex_recv;
	std::mutex m_mutex_send;
	std::condition_variable m_cond_var_recv; // ȫ����������
	std::condition_variable m_cond_var_send; // ȫ����������
	bool m_is_empty=true;
	
};




class NetworkClient
{
public:
	NetworkClient(std::string dst_address,int dst_port, int local_port=0,std::string start_c=":", std::string end_c="\x0D\x0A",int msg_protocol=0);
	~NetworkClient();

	//������Ϣ������
	int recv_message(std::string &dst);

	//������Ϣ��������
	int recv_message_no_block(std::string &dst);

	int message_num_to_recv();


	int send_message(std::string &msg);


	static std::string decode_data_hex(std::string &src_data);
	static std::string encode_data_hex(std::string &src_data);


	//�̺߳���
	static void client_process_recv(void* p_parent);
	static void client_process_send(void* p_parent);
	//�ڲ�ʹ��
public:
	int _push_one_message(std::string &msg);
	int _acquire_one_message(std::string &msg);
	int _acquire_one_message_no_block(std::string &msg);
	int _start_run();
	int _is_big_endien();

public:
	int m_protocol = 0;
	std::string m_start_c;
	std::string m_end_c;
	std::string m_dst_address;
	int m_dst_port;
	int m_local_port;
	std::atomic_bool m_is_connected = 0;
	bool m_is_running = true;
	SocketLib::DataSocket *m_pSocket;
	bool m_is_big_endien = false;

protected:
	const int max_message_num = 512;
	std::vector<std::string> m_messages_send;
	std::vector<std::string> m_messages_recv;

	std::mutex m_mutex_recv;
	std::mutex m_mutex_send;
	std::condition_variable m_cond_var_recv; // ȫ����������
	std::condition_variable m_cond_var_send; // ȫ����������

	std::thread *pMainwoker_recv=nullptr;
	std::thread *pMainwoker_send=nullptr;

};
