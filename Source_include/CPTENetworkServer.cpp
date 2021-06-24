#include "CPTENetworkServer.h"
#include "EasySocket.h"
#include "thread"
#include "SocketErrors.h"
#include <iostream>
#include <sstream>
#include "cstring"



#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#endif // _WIN32

using namespace SocketLib;


size_t find_full_string(const std::string &srcstr, const std::string &cmpstr)
{
	if (srcstr.size() < cmpstr.size())
	{
		return std::string::npos;
	}
	for (size_t i=0;i<=(srcstr.size()-cmpstr.size());i++)
	{
		if (srcstr[i]==cmpstr[0])
		{
			bool isok = true;
			for (size_t j=1;j<cmpstr.size();j++)
			{
				if (srcstr[i+j] != cmpstr[j])
				{
					isok = false;
					break;
				}
			}
			if (isok)
			{
				return i;
			}
		}
	}
	return std::string::npos;
}




void work_process_send(void *pParent, DataSocket sock, int socket_id)
{
	NetworkServer *parent = (NetworkServer*)pParent;
	//DataSocket sock = (DataSocket*)psock;

	//parent->logger->info() 
#ifdef DEBUG_NETWORK
	std::cout << "enter thread send client:" << socket_id;
#endif // DEBUG_NETWORK

	long buffer_length = 1024 * 1024;
	char* pbuffer = new char[buffer_length];
	std::string start_c = parent->m_start_c;
	std::string end_c = parent->m_end_c;
	std::string msgdata;
	std::string send_data;
	long sz_data=0;
	bool do_break = false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	int try_times = 0;
	int msg_protocol = parent->m_protocol;
	while (true)
	{
		if (!parent->is_running) return;
		if (do_break) break;
		sz_data = 0;
		//������Ϣ
		int res = parent->_acquire_one_message(send_data, socket_id);
		if (res)
		{
			//std::cout << "sever get one message to send" << std::endl;
			if (!send_data.empty())
			{
				if (msg_protocol==CHINA_POST_PROTO)
				{
					if (!start_c.empty()) send_data = start_c + send_data;
					if (!end_c.empty()) send_data = send_data + end_c;
				}

				int try_send_num = 0;
				sz_data = 0;
				while (true)
				{
					try
					{
						try_send_num += 1;
						sz_data = sock.Send(send_data.c_str(), send_data.size());
					}
					catch (Exception&e)
					{
						//std::cout << e.PrintError() << std::endl;
						if (try_send_num<100)
						{
							std::this_thread::sleep_for(std::chrono::milliseconds(1));
							continue;
						}
#ifdef DEBUG_NETWORK
						std::cout << "client:" << socket_id << " lost connection: " << e.PrintError();
#endif // DEBUG_NETWORK

						do_break = true; //�����쳣���˳��߳�
						break;
					}
					try_send_num = 0;
					if (sz_data == send_data.size())
					{
#ifdef DEBUG_NETWORK
						std::cout << "sever send over:"<< send_data << std::endl;
#endif // DEBUG_NETWORK
						//
						break;
					}
					send_data = send_data.substr(sz_data); //û�з�����ɼ�������
					if (send_data.empty()) break;

				}
			}

		}
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	parent->_close_one_client(socket_id);
	sock.Close();
	//parent->logger->info() << "leave thread send client:" << socket_id;
	delete [] pbuffer;
}


void work_process_recv(void *pParent, DataSocket sock, int socket_id)
{
	
	

	NetworkServer *parent = (NetworkServer*)pParent;
	//DataSocket sock = (DataSocket*)psock;
#ifdef DEBUG_NETWORK
	std::cout<< "enter thread recv client:" << socket_id;
#endif // DEBUG_NETWORK

	//parent->logger->info() 
	//std::cout << "enter thread recv msg" << std::endl;
	long buffer_length = 1024 * 1024;
	char* pbuffer = new char[buffer_length];
	std::string start_c = parent->m_start_c;
	std::string end_c = parent->m_end_c;
	std::string msgdata;
	char tc[2];
	bool is_big_endig = parent->m_is_big_endien;
	long sz_data = 0;
	bool do_break = false;
	int try_times = 0;
	int msg_protocol = parent->m_protocol;
	while (true)
	{
		if (!parent->is_running) return;
		if (do_break) break;
		sz_data = 0;
		//������Ϣ
		try
		{
			//std::cout << "server waiting to recv:" << std::endl;
			try_times += 1;
			sz_data = sock.Receive(pbuffer, buffer_length);
		}
		catch (Exception& e)
		{
			if (e.ErrorCode() == Error::EOperationWouldBlock || e.ErrorCode() == Error::ETimedOut)
			{}
			else
			{
				if (try_times > 100)
				{
#ifdef DEBUG_NETWORK
					//std::cout << "Warning! " << socket_id << " connect error: " << e.PrintError() << std::endl;;
					std::cout << "client:" << socket_id <<" lost connection: "<< e.PrintError();
#endif // DEBUG_NETWORK


					do_break = true;
					continue;
				}
				//�κ����������˳��߳�
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

		}
		try_times = 0;
		if (sz_data > 0)
		{
			//������ʼ��ֹ������������Ϣ
			//parent->logger->debug() << "client "<< socket_id << " recv a raw msg, length:"<< sz_data;

			if (msg_protocol == SF_EXPRESS_PROTO) //顺丰格式消息
			{
				std::string ss(pbuffer, sz_data);
				//std::cout << "sever recv data:" << ss << std::endl;
				msgdata += ss;
				std::string one_msg;
				std::string start_flag("\xFF\xFF");
				while (true)
				{
					size_t pos1 = msgdata.find_first_of(start_flag);
					if (pos1 == std::string::npos)
					{
						//parent->logger->debug() << "client " << socket_id << " not a valid msg:"<< NetworkServer::encode_data_hex(msgdata);
						msgdata = "";
						break;
					}
					msgdata = msgdata.substr(pos1);

					unsigned short *pNum;

					if (is_big_endig)
					{
						pNum = (unsigned short *)(msgdata.c_str() + 2);
					}
					else
					{
						tc[0] = *(msgdata.c_str() + 3);
						tc[1] = *(msgdata.c_str() + 2);
						pNum = (unsigned short *)tc;
					}

					if (*pNum > msgdata.size()) {
						//parent->logger->warning() << "client " << socket_id << " msg length is wrong:";
						break;
					}
					one_msg = msgdata.substr(0, *pNum);
					parent->_push_one_message(one_msg, socket_id);
					msgdata = msgdata.substr(*pNum);
				}
			}
			if (msg_protocol == CHINA_POST_PROTO) //中国邮政格式消息
			{
				std::string ss(pbuffer, sz_data);
#ifdef DEBUG_NETWORK
				std::cout << "sever recv data: " << ss << std::endl;
#endif // DEBUG_NETWORK

				msgdata += ss;
				std::string one_msg;
				if (!start_c.empty() && !end_c.empty())
				{
					while (true)
					{
						size_t pos1 = find_full_string(msgdata, start_c);
						if (pos1 == std::string::npos)
						{
							msgdata = "";
#ifdef DEBUG_NETWORK
							std::cout << "Not find start marker" << std::endl;
#endif // DEBUG_NETWORK

							//parent->logger->debug() << "client " << socket_id << " not a valid msg";
							break;
						}
						msgdata = msgdata.substr(pos1);
						size_t pos2 = find_full_string(msgdata, end_c);
						if (pos2 == std::string::npos) {
							//parent->logger->debug() << "client " << socket_id << " msg length is wrong
#ifdef DEBUG_NETWORK
							std::cout << "Not find end marker" << std::endl;
#endif // DEBUG_NETWORK
							break;
						}
						one_msg = msgdata.substr(start_c.size(), pos2 - 1);
						parent->_push_one_message(one_msg, socket_id);
						if (pos2 + end_c.size() < msgdata.size())
						{
							msgdata = msgdata.substr(pos2 + end_c.size());
						}
						else
						{
							msgdata = "";
							break;
						}
					}
				}
				else
				{
					one_msg = msgdata;
					parent->_push_one_message(one_msg, socket_id);
					msgdata = "";
				}
			}

		}

	}
	parent->_close_one_client(socket_id);
	sock.Close();
	//parent->logger->info() << "leave thread recv client:" << socket_id;
	delete [] pbuffer;
}


void ip_to_string(ipaddress i_ip, std::string& ip)
{
	unsigned int temp_byte;
	std::ostringstream	oss;

	// Convert the IP from unsigned int to string

	for (int index = 0; index < 4; index++)
	{
		temp_byte = i_ip;
		temp_byte >>= index * 8;
		temp_byte &= 255;

		oss << temp_byte;

		if (index != 3)
		{
			oss << ".";
		}
	}

	ip = oss.str();
}



//��ͻ���
void NetworkServer::main_process(void *parent, uint16_t in_port)
{
	NetworkServer* pparent = (NetworkServer*)parent;
	//pparent->logger->info() << "listen port:" << in_port;

	ListeningSocket lsock;
	
	try
	{
		lsock.Listen(in_port);
	}
	catch (Exception& e)
	{
		//pparent->logger->critic() << "listen error: exit..."<< e.PrintError();
		abort();
		return;
	}
	//std::vector<DataSocket> pdsocks;
	int thread_id_now=0;
	pparent->m_pLSock = &lsock;
	//std::vector<DataSocket> &socks_vec =;
	while (true)
	{
		if (!pparent->is_running) break;
		//pparent->logger->info() << "waiting connection";
		DataSocket dsock;
		try
		{
			dsock = lsock.Accept();
		}
		catch (Exception& e)
		{
			//pparent->logger->warning() << "accept error: " << e.PrintError();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		
		//std::cout << "action"<< std::endl;
		if (dsock.IsConnected())
		{
			//dsock.SetBlocking(false);
			std::string ip;
			ip_to_string(dsock.GetRemoteAddress(), ip);
#ifdef DEBUG_NETWORK
			std::cout<< "get a client:" << ip << ", client_id:" << thread_id_now;
#endif // DEBUG_NETWORK

			//pparent->logger->notice()

			auto pthread = new std::thread(work_process_recv,parent, dsock, thread_id_now);
			pparent->m_client_threads.push_back(pthread);
			pthread = new std::thread(work_process_send, parent, dsock, thread_id_now);
			pparent->m_client_threads.push_back(pthread);

			pparent->_register_one_client();
			pparent->m_DSocks.push_back(dsock);
			thread_id_now += 1;
		}
	}
	//pparent->logger->critic() << "Network main thread is interrupted";
}


NetworkServer::NetworkServer(uint16_t in_port):m_start_c(0),m_end_c(0),m_in_port(in_port), m_protocol(0)
{
	this->_start_run();
}

NetworkServer::NetworkServer(uint16_t in_port, std::string start_c /*= 2*/, std::string end_c /*= 3*/, int protocol):m_start_c(start_c),m_end_c(end_c),m_in_port(in_port), m_protocol(protocol)
{
	this->_start_run();
}


NetworkServer::~NetworkServer()
{
	//if (logger) delete logger;
	is_running = false;
	if (m_pLSock)
	{
		m_pLSock->Close();
	}
	for (int i = 0; i < m_DSocks.size(); i++)
	{
		if (m_DSocks[i].IsConnected())
		{
			m_DSocks[i].Close();
		}
	}
	for (int i = 0; i < m_client_threads.size(); i++)
	{
		delete m_client_threads[i];
	}
	if (pMainwoker)
	{
		delete pMainwoker;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	delete this->pMainwoker;
}

int NetworkServer::recv_one_message(std::string &dst, client_id &cid)
{
	std::pair<std::string, client_id> tmp;
	std::unique_lock <std::mutex> lck(this->m_mutex_recv);
	if (m_messages_recv.empty())
	{
		while (m_messages_recv.empty())
		{
			this->m_cond_var_recv.wait(lck);
			if (!m_messages_recv.empty())
			{
				tmp = m_messages_recv.front();
				m_messages_recv.erase(m_messages_recv.begin());
				break;
			}
		}
	}
	else
	{
		tmp = m_messages_recv.front();
		m_messages_recv.erase(m_messages_recv.begin());
	}
	dst = tmp.first;
	cid = tmp.second;
	return 1;
}


int NetworkServer::recv_one_message_no_block(std::string &dst, client_id &cid)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_recv);
	if (m_messages_recv.empty()) return 0;
	auto tmp = m_messages_recv.front();
	m_messages_recv.erase(m_messages_recv.begin());
	dst = tmp.first;
	cid = tmp.second;
	return 1;
}

int NetworkServer::message_num_to_recv()
{
	std::unique_lock <std::mutex> lck(this->m_mutex_recv);
	return m_messages_recv.size();
}

int NetworkServer::send_message_to_one(std::string &msg, client_id cid)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	if (m_clients_iswork.size() <= cid) return 0;
	if (m_clients_iswork[cid] == false) return 0;
	std::pair<std::string, client_id> tmp;
	tmp.first = msg;
	tmp.second = cid;
	m_messages_send.push_back(tmp);
	m_flag_message_send[cid] += 1;
	m_cond_var_send.notify_all();
	return 1;
}

int NetworkServer::send_message_to_all(std::string msg)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	std::pair<std::string, client_id> tmp;
	for (int i=0;i< m_clients_iswork.size();i++)
	{
		if (m_clients_iswork[i]==true)
		{
			tmp.first = msg;
			tmp.second = i;
			m_messages_send.push_back(tmp);
		}
	}
	m_cond_var_send.notify_all();
	return 1;
}

std::string NetworkServer::decode_data_hex(std::string &src_data)
{
	uchar byteData;
	uchar pos_data;
	std::string decode_data;

	if ((src_data.size() % 2) != 0)
	{
#ifdef DEBUG_NETWORK
		std::cout << "decode src data size is invalid:" << src_data.size() << std::endl;
#endif // DEBUG_NETWORK

		return "";
	}
	for (int i = 0; i < src_data.size(); i = i + 2)
	{
		// ��һ�ֽ�
		byteData = src_data[i];
		if ((byteData >= '0') && (byteData <= '9'))
		{
			byteData -= 0x30;
			pos_data = byteData << 4;
		}
		else if ((byteData >= 'A') && (byteData <= 'F'))
		{
			byteData -= 0x37;
			pos_data = byteData << 4;
		}
		else
		{
			continue;
		}

		// �ڶ��ֽ�
		byteData = src_data[i + 1];
		if ((byteData >= '0') && (byteData <= '9'))
		{
			byteData -= 0x30;
			pos_data |= byteData;
		}
		else if ((byteData >= 'A') && (byteData <= 'F'))
		{
			byteData -= 0x37;
			pos_data |= byteData;
		}
		else
		{
			continue;
		}
		decode_data.push_back(pos_data);
	}
	return decode_data;
}


std::string NetworkServer::encode_data_hex(std::string &src_data)
{
	int i_S = 0;
	int i_B = 0;
	static const char hexes[17] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F','\0' };

	unsigned char a;
	int m_count = 0;
	std::string encoded_data;
	std::string::iterator it;
	for (it = src_data.begin(); it != src_data.end(); it++)
	{
		a = *it;
		unsigned int i_l = a >> 4;
		unsigned int i_r = a & 0x0F;
		encoded_data.push_back(hexes[i_l]);
		encoded_data.push_back(hexes[i_r]);

	}
	return encoded_data;
}

bool NetworkServer::check_msg_validation_china_post(std::string &src_data)
{
	if (src_data.size()<2)
	{
		return false;
	}
	uchar check = 0;
	for (int i=0;i<src_data.size()-1;i++)
	{
		check += (uchar)src_data[i];
	}
	check = ~check;
	check++;


	return (check == (uchar)src_data[src_data.size() - 1]);
}

unsigned char NetworkServer::generate_msg_validation_code_china_post(const std::string &src_data)
{
	uchar check = 0;
	for (int i = 0; i < src_data.size(); i++)
	{
		check += (uchar)src_data[i];
	}
	check = ~check;
	check++;
	return check;
}

int NetworkServer::_push_one_message(std::string msg, client_id cid)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_recv);
	if (m_messages_recv.size() >= max_message_num)
	{
		return 0;
	}
	std::pair<std::string, client_id> tmp;
	tmp.first = msg;
	tmp.second = cid;
	this->m_messages_recv.push_back(tmp);
	m_cond_var_recv.notify_all();
	return 1;
}

int NetworkServer::_acquire_one_message(std::string &msg, client_id cid)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	while (m_flag_message_send[cid] == 0 && is_running)
	{
		this->m_cond_var_send.wait(lck);
	}
	if (!is_running)
	{
		return 0;
	}
	m_flag_message_send[cid] -= 1;
	if (m_flag_message_send[cid] < 0) m_flag_message_send[cid] = 0;


	std::vector<std::pair<std::string, client_id>>::iterator it;
	for (it=m_messages_send.begin();it!=m_messages_send.end();it++)
	{
		if (it->second == cid)
		{
			msg = it->first;
			m_messages_send.erase(it);
			break;
		}
	}
	return 1;
}

int NetworkServer::_acquire_one_message_no_block(std::string &msg, client_id cid)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	if (m_messages_send.empty()) return 0;
	std::vector<std::pair<std::string, client_id>>::iterator it;
	if (m_flag_message_send[cid]==0)
	{
		return 0;
	}
	for (it = m_messages_send.begin(); it != m_messages_send.end(); it++)
	{
		if (it->second == cid)
		{
			msg = it->first;
			m_messages_send.erase(it);
			break;
		}
	}
	return 1;
}

int NetworkServer::_register_one_client()
{
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	m_clients_iswork.push_back(true);
	m_flag_message_send.push_back(0);
	return 1;
}


int NetworkServer::_close_one_client(client_id cid)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	if (m_clients_iswork.size() <= cid) return 1;
	m_clients_iswork[cid] = false;
	return 1;
}

int NetworkServer::_start_run()
{
	//if (logger == nullptr) logger = new Log::Logger("Network");

// 	m_is_big_endien = _is_big_endien();
// 	if (m_is_big_endien) 
// 	{
// 		//logger->info() << "system is big endian";
// 	}
// 	else
// 	{
// 		//logger->info() << "system is small endian";
// 	}

	pMainwoker = new std::thread(main_process, this, m_in_port);
	return 1;
}



int NetworkServer::_is_big_endien()
{
	int a = 0x1234;
	char c = static_cast<char>(a);
	if (c == 0x12)
		return 1;
	else if (c == 0x34)
		return 0;
}

NetworkClient::NetworkClient(std::string dst_address, int dst_port, int local_port,std::string start_c/*=2*/, std::string end_c/*=3*/,int protocol):m_dst_address(dst_address),\
							m_dst_port(dst_port),m_start_c(start_c),m_end_c(end_c),m_protocol(protocol),m_local_port(local_port)
{
	_start_run();
}

NetworkClient::~NetworkClient()
{
	m_is_running = false;
	if (m_pSocket->IsConnected())
	{
		m_pSocket->Close();
	}
	m_cond_var_recv.notify_all();
	m_cond_var_send.notify_all();
	if (pMainwoker_recv != nullptr)
	{
		delete pMainwoker_recv;
	}
	if (pMainwoker_send != nullptr)
	{
		delete pMainwoker_send;
	}
	if (m_pSocket != nullptr)
	{
		delete (DataSocket*)m_pSocket;
	}
}

int NetworkClient::recv_message(std::string &dst)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_recv);
	while (m_messages_recv.empty() && m_is_running)
	{
		m_cond_var_recv.wait(lck);
	}
	if (!m_is_running)
	{
		return 0;
	}
	dst = m_messages_recv.front();
	m_messages_recv.erase(m_messages_recv.begin());
	return 1;
}

int NetworkClient::recv_message_no_block(std::string &dst)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_recv);
	if (m_messages_recv.empty()) return 0;
	dst = m_messages_recv.front();
	m_messages_recv.erase(m_messages_recv.begin());
	return 1;
}

int NetworkClient::message_num_to_recv()
{
	std::unique_lock <std::mutex> lck(this->m_mutex_recv);
	return m_messages_recv.size();
}

int NetworkClient::send_message(std::string &msg)
{
	if (!m_is_connected) return 0;
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	m_messages_send.push_back(msg);
	m_cond_var_send.notify_all();
	return 1;
}

std::string NetworkClient::decode_data_hex(std::string &src_data)
{
	return NetworkServer::decode_data_hex(src_data);
}

std::string NetworkClient::encode_data_hex(std::string &src_data)
{
	return NetworkServer::encode_data_hex(src_data);
}

int NetworkClient::_push_one_message(std::string &msg)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_recv);
	m_messages_recv.push_back(msg);
	m_cond_var_recv.notify_all();
	return 1;
}

int NetworkClient::_acquire_one_message(std::string &msg)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	while (m_messages_send.empty() && m_is_running)
	{
		m_cond_var_send.wait(lck);
	}
	if (!m_is_running)
	{
		return 0;
	}
	msg = m_messages_send.front();
	m_messages_send.erase(m_messages_send.begin());
	return 1;
}

int NetworkClient::_acquire_one_message_no_block(std::string &msg)
{
	std::unique_lock <std::mutex> lck(this->m_mutex_send);
	if (m_messages_send.empty()) return 0;
	msg = m_messages_send.front();
	m_messages_send.erase(m_messages_send.begin());
	return 1;
}


int NetworkClient::_start_run()
{
	m_is_big_endien = _is_big_endien();
	if (m_is_big_endien)
	{
		//logger->info() << "system is big endian";
	}
	else
	{
		//logger->info() << "system is small endian";
	}

	m_pSocket = new DataSocket();
	pMainwoker_recv = new std::thread(client_process_recv, (void*)(this));
	pMainwoker_send = new std::thread(client_process_send, (void*)(this));
	return 1;
}

int NetworkClient::_is_big_endien()
{
	int a = 0x1234;
	char c = static_cast<char>(a);
	if (c == 0x12)
		return 1;
	else if (c == 0x34)
		return 0;
}

void NetworkClient::client_process_recv(void* p_parent)
{

	NetworkClient* parent = (NetworkClient*)p_parent;
	//std::cout << "enter client thread:" << pid << std::endl;
	const size_t sz_data = 1024 * 1024;
	char *pbuffer = new char[sz_data];
	std::string start_c = parent->m_start_c;
	std::string end_c = parent->m_end_c;
	char tc[2];
	DataSocket *psocket = (DataSocket *)parent->m_pSocket;
	bool is_big_endig = parent->m_is_big_endien;
	std::string msgdata;
	int msg_protocol = parent->m_protocol;
	int binded = 0;
	while (true)
	{
		parent->m_is_connected = false;
		if (!parent->m_is_running)
		{
			break;
		}
		if (!binded && parent->m_local_port!=0)//绑定端口
		{
			try
			{
				psocket->Bind("", parent->m_local_port);
			}
			catch (Exception& e)
			{
				std::cout <<"Bing error: "<< e.PrintError() << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}
			binded = 1;
		}
		try
		{
			psocket->Connect(parent->m_dst_address, parent->m_dst_port);
		}
		catch (Exception& e)
		{
			if (e.ErrorCode() == Error::EAlreadyConnected)
			{
			}
			else
			{
				std::cout <<"Connection error:"<< e.PrintError() << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				continue;
			}
		}
		if(psocket->IsConnected())
		{
			parent->m_is_connected = true;
			std::cout << "local port:" << psocket->GetLocalPort() << std::endl;
			std::cout << "connect to server:" << parent->m_dst_address << ":" << parent->m_dst_port << std::endl;
			unsigned int try_times = 0;
			while (true)
			{
				int res = 0;

				try
				{
					try_times += 1;
					res = psocket->Receive(pbuffer, sz_data);
				}
				catch (Exception& e)
				{
					if (e.ErrorCode() == Error::ETimedOut)
					{
						continue;
					}
					if (try_times<100)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(5));
						continue;
					}
#ifdef DEBUG_NETWORK
					std::cout <<"lost connection from server, "<< e.PrintError() << std::endl;
#endif // DEBUG_NETWORK
					//psocket->Close();
					
					break;
				}
				try_times = 0;
				if (res > 0)
				{
					if (msg_protocol == CHINA_POST_PROTO)
					{
						std::string ss(pbuffer, res);
#ifdef DEBUG_NETWORK
						std::cout << "client recv data:" << ss << std::endl;
#endif // DEBUG_NETWORK

						//
						msgdata += ss;
						std::string one_msg;
						while (true)
						{
							size_t pos1 = find_full_string(msgdata, start_c);
							if (pos1 == std::string::npos)
							{
								msgdata = "";
#ifdef DEBUG_NETWORK
								std::cout << "Not find start marker" << std::endl;
#endif // DEBUG_NETWORK
								break;
							}
							msgdata = msgdata.substr(pos1);
							size_t pos2 = find_full_string(msgdata, end_c);
							if (pos2 == std::string::npos)
							{
#ifdef DEBUG_NETWORK
								std::cout << "Not find end marker" << std::endl;
#endif // DEBUG_NETWORK
								break;
							}
							one_msg = msgdata.substr(start_c.size(), pos2 - 1);
							parent->_push_one_message(one_msg);
							if (pos2 + end_c.size() < msgdata.size())
							{
								msgdata = msgdata.substr(pos2 + end_c.size());
							}
							else
							{
								msgdata = "";
								break;
							}
						}
					}
					else if (msg_protocol == SF_EXPRESS_PROTO)
					{
						std::string ss(pbuffer, res);
#ifdef DEBUG_NETWORK
						std::cout << "client recv data:" << ss << std::endl;
#endif // DEBUG_NETWORK

						//
						msgdata += ss;
						std::string one_msg;
						std::string start_flag("\xFF\xFF");
						while (true)
						{
							size_t pos1 = find_full_string(msgdata, start_flag);
							if (pos1 == std::string::npos)
							{
#ifdef DEBUG_NETWORK
								std::cout << "received not a valid msg, hex:" << NetworkServer::encode_data_hex(msgdata);
#endif // DEBUG_NETWORK

								//parent->logger->debug() 
								msgdata = "";
								break;
							}
							msgdata = msgdata.substr(pos1);

							unsigned short *pNum;

							if (is_big_endig)
							{
								pNum = (unsigned short *)(msgdata.c_str() + 2);
							}
							else
							{
								tc[0] = *(msgdata.c_str() + 3);
								tc[1] = *(msgdata.c_str() + 2);
								pNum = (unsigned short *)tc;
							}

							if (*pNum > msgdata.size()) {
								//parent->logger->warning() << "client " << socket_id << " msg length is wrong:";
								break;
							}
							one_msg = msgdata.substr(0, *pNum);
							parent->_push_one_message(one_msg);
							msgdata = msgdata.substr(*pNum);
						}
					}

				}
			}
		}
		try
		{
			binded = 0;
			psocket->Close();
		}
		catch (Exception&e)
		{
			std::cout << "Close error:" << e.PrintError() << std::endl;
		}
	}
}

void NetworkClient::client_process_send(void* p_parent)
{

	NetworkClient* pparent = (NetworkClient*)p_parent;
	//std::cout << "enter client thread:" << pid << std::endl;

	int msg_protocol = pparent->m_protocol;
	std::string start_c = pparent->m_start_c;
	std::string end_c = pparent->m_end_c;
	DataSocket *psocket = (DataSocket *)pparent->m_pSocket;
	std::string msgdata;
	bool check_state = false;
	while (true)
	{
		if (!pparent->m_is_running) break;
		if (!pparent->m_is_connected)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		else
		{
			check_state = false;
			unsigned int try_times = 0;
			while (true)
			{
				if (check_state==true) break;
				int res = pparent->_acquire_one_message(msgdata);
				if (res)
				{
					//std::cout << "sever get one message to send" << std::endl;

					if (!msgdata.empty())
					{
						if (msg_protocol==CHINA_POST_PROTO)
						{
							if (!start_c.empty()) msgdata = start_c + msgdata;
							if (!end_c.empty()) msgdata += end_c;
						}
						int try_send_num = 0;
						int sz_data = 0;
						while (true)
						{
							try
							{
								try_send_num += 1;
								sz_data = psocket->Send(msgdata.c_str(), msgdata.size());
							}
							catch (Exception&e)
							{
								//std::cout << e.PrintError() << std::endl;
								if (try_send_num < 100)
								{
									std::this_thread::sleep_for(std::chrono::milliseconds(1));
									continue;
								}
#ifdef DEBUG_NETWORK
								std::cout <<"send data exception, "<< e.PrintError() << std::endl;
#endif // DEBUG_NETWORK

								
								check_state = true;
								break;
							}
							try_send_num = 0;
							if (sz_data == msgdata.size())
							{
#ifdef DEBUG_NETWORK
								if (msg_protocol==SF_EXPRESS_PROTO)
								{
									std::cout << "send a msg over, length:"<<sz_data<<", hex: " << NetworkServer::encode_data_hex(msgdata) << std::endl;
								}
								else
								{
									std::cout << "send a msg over, length:" << sz_data<<", data:" << msgdata << std::endl;
								}
								
#endif // DEBUG_NETWORK
								//
								break;
							}
							msgdata = msgdata.substr(sz_data); //û�з�����ɼ�������
							if (msgdata.empty()) break;
						}
					}
				}
			}
		}
	}
}
