#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>

#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include "protocol.h"

constexpr int RANGE = 11;

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;
//constexpr int MAX_USER = 1000;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};

enum SESSION_STATE { ST_FREE, ST_ACCEPTED, ST_INGAME};

class SESSION {
	OVER_EXP _recv_over;

public:
	mutex	_sl;
	SESSION_STATE _s_state;
	int _id;
	SOCKET _socket;
	short	x, y;
	char	_name[NAME_SIZE];
	unordered_set <int> view_list;
	mutex vl;
	int		_prev_remain;
public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = 0;
		_name[0] = 0;
		_s_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, 0);
	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}
	void send_login_info_packet()
	{
		SC_LOGIN_OK_PACKET p;
		p.id = _id;
		p.size = sizeof(SC_LOGIN_OK_PACKET);
		p.type = SC_LOGIN_OK;
		p.x = x;
		p.y = y;
		do_send(&p);
	}
	void send_move_packet(int c_id, int client_time);

	// 로그인 할 때 뿐 아니라 시야에 들어오면 addPlayer패킷을 보냄
	void send_addPlayer_packet(int my_id, int newPlayer_id);

	// 시야에서 사라지면 removePlayer패킷 보내는 함수
	void send_removePlayer_packet(int my_id, int RemovePlayer_id);
};

array<SESSION, MAX_USER> clients;
HANDLE g_h_iocp;
SOCKET g_s_socket;

int distance(int a, int b)
{
	return abs(clients[a].x - clients[b].x) + abs(clients[a].y - clients[b].y);
}

void SESSION::send_move_packet(int c_id, int client_time)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = clients[c_id].x;
	p.y = clients[c_id].y;
	p.client_time = client_time;
	do_send(&p);
}

void disconnect(int c_id);
int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		clients[i]._sl.lock();
		if (clients[i]._s_state == ST_FREE) {
			clients[i]._s_state = ST_ACCEPTED;
			clients[i]._sl.unlock();
			return i;
		}
		clients[i]._sl.unlock();
	}
	return -1;
}

void SESSION::send_addPlayer_packet(int my_id, int newPlayer_id)
{

}

void SESSION::send_removePlayer_packet(int my_id, int RemovePlayer_id)
{

}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		clients[c_id]._sl.lock();
		if (clients[c_id]._s_state == ST_FREE) {
			clients[c_id]._sl.unlock();
			break;
		}
		if (clients[c_id]._s_state == ST_INGAME) {
			clients[c_id]._sl.unlock();
			disconnect(c_id);
			break;
		}

		strcpy_s(clients[c_id]._name, p->name);
		clients[c_id].send_login_info_packet();
		clients[c_id]._s_state = ST_INGAME;
		clients[c_id]._sl.unlock();

		//clients[c_id].x = rand() % W_WIDTH;
		//clients[c_id].y = rand() % W_HEIGHT;
		clients[c_id].x = 0;
		clients[c_id].y = 0;

		for (auto& pl : clients) {
			if (pl._id == c_id) continue;
			pl._sl.lock();
			if (ST_INGAME != pl._s_state) {
				pl._sl.unlock();
				continue;
			}

			if (RANGE >= distance(c_id, pl._id)) {
				pl.vl.lock();
				pl.view_list.insert(c_id);
				pl.vl.unlock();
				SC_ADD_OBJECT_PACKET add_packet;
				add_packet.id = c_id;
				strcpy_s(add_packet.name, p->name);
				add_packet.size = sizeof(add_packet);
				add_packet.type = SC_ADD_OBJECT;

				add_packet.x = clients[c_id].x;
				add_packet.y = clients[c_id].y;

				pl.do_send(&add_packet);
			}

			pl._sl.unlock();
		}
		for (auto& pl : clients) {
			if (pl._id == c_id) continue;
			lock_guard<mutex> aa {pl._sl};
			if (ST_INGAME != pl._s_state) continue;

			if (RANGE >= distance(pl._id, c_id)) {
				clients[c_id].vl.lock();
				clients[c_id].view_list.insert(pl._id);
				clients[c_id].vl.unlock();
				SC_ADD_OBJECT_PACKET add_packet;
				add_packet.id = pl._id;
				strcpy_s(add_packet.name, pl._name);
				add_packet.size = sizeof(add_packet);
				add_packet.type = SC_ADD_OBJECT;
				add_packet.x = pl.x;
				add_packet.y = pl.y;
				clients[c_id].do_send(&add_packet);
			}
		}
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		short x = clients[c_id].x;
		short y = clients[c_id].y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}
		clients[c_id].x = x;
		clients[c_id].y = y;

		clients[c_id].vl.lock();
		// 이전의 vl를 저장하는 set
		unordered_set<int> oldVL = clients[c_id].view_list;
		clients[c_id].vl.unlock();

		
		//for (auto& pl : clients) {
		//	lock_guard<mutex> aa{ pl._sl };
		//	if (ST_INGAME == pl._s_state) 
		//		pl.send_move_packet(c_id, p->client_time);
		//	}

		clients[c_id].send_move_packet(c_id, p->client_time);

		// 최근의 vl를 저장하는 set, 
		// 만약 old에 없었는데 여기 있으면 새롭게 접속
		// old에 있었는데 여기 없으면 접속 끊기
		// old에 있고 여기도 있으면 이동
		unordered_set<int> newVL;
		for (auto& pl : clients) {
			if (pl._id == c_id) continue;
			//pl._sl.lock();
			if (ST_INGAME != pl._s_state) {
				//pl._sl.unlock();
				continue;
			}

			if (RANGE >= distance(c_id, pl._id))
				newVL.insert(pl._id);
		}

		for (auto& playerindex : newVL)
		{
			// old에는 없고 new에는 있다 -> 새롭게 화면에 들어온 플레이어다 -> AddPlayer패킷 보내기
			if (oldVL.count(playerindex) == 0)
			{
				SC_ADD_OBJECT_PACKET add_packet;
				add_packet.id = playerindex;
				strcpy_s(add_packet.name, clients[playerindex]._name);
				add_packet.size = sizeof(add_packet);
				add_packet.type = SC_ADD_OBJECT;

				add_packet.x = clients[playerindex].x;
				add_packet.y = clients[playerindex].y;

				clients[c_id].do_send(&add_packet);
				clients[c_id].view_list.insert(playerindex);
			}
			// old에도 있고, new에도 있다 -> 기존의 화면에 있던 플레이어다 -> move 패킷 보내기
			else
			{
				// 만약 상대방의 viewlist에 없다면 AddPlayer해줌
				if (clients[playerindex].view_list.count(c_id) == 0)
				{
					SC_ADD_OBJECT_PACKET add_packet;
					add_packet.id = c_id;
					strcpy_s(add_packet.name, clients[c_id]._name);
					add_packet.size = sizeof(add_packet);
					add_packet.type = SC_ADD_OBJECT;

					add_packet.x = clients[c_id].x;
					add_packet.y = clients[c_id].y;

					clients[playerindex].do_send(&add_packet);

					clients[playerindex].view_list.insert(c_id);
				}
				else	// 있다면 move
				{
					CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
					clients[playerindex].send_move_packet(c_id, p->client_time);
				}
				
			}
		}
		for (auto playerindex : oldVL)
		{
			// newVL에 없고 oldVL에 있다 -> 화면에서 벗어남 -> remove 패킷 보내기
			if (newVL.count(playerindex) == 0)
			{
				SC_REMOVE_OBJECT_PACKET p;
				p.id = playerindex;
				p.size = sizeof(p);
				p.type = SC_REMOVE_OBJECT;
				clients[c_id].do_send(&p);

				clients[c_id].vl.lock();
				clients[c_id].view_list.erase(playerindex);
				clients[c_id].vl.unlock();
			}
		}
		break;
	}
	}
}

void disconnect(int c_id)
{
	clients[c_id]._sl.lock();
	if (clients[c_id]._s_state == ST_FREE) {
		clients[c_id]._sl.unlock();
		return;
	}
	closesocket(clients[c_id]._socket);
	clients[c_id]._s_state = ST_FREE;

	// 플레이어 접속이 끊길 때 viewlist에서 제거
	clients[c_id].vl.lock();
	clients[c_id].view_list.erase(c_id);
	clients[c_id].vl.unlock();
	///////////////////////////////////////////

	clients[c_id]._sl.unlock();

	for (auto& pl : clients) {
		if (pl._id == c_id) continue;
		pl._sl.lock();
		if (pl._s_state != ST_INGAME) {
			pl._sl.unlock();
			continue;
		}

		SC_REMOVE_OBJECT_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_OBJECT;
		pl.do_send(&p);
		pl._sl.unlock();
	}
}

void do_worker()
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			SOCKET c_socket = reinterpret_cast<SOCKET>(ex_over->_wsabuf.buf);
			int client_id = get_new_client_id();
			if (client_id != -1) {
				clients[client_id].x = 0;
				clients[client_id].y = 0;
				clients[client_id]._id = client_id;
				clients[client_id]._name[0] = 0;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket),
					g_h_iocp, client_id, 0);
				clients[client_id].do_recv();
				c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&ex_over->_over, sizeof(ex_over->_over));
			ex_over->_wsabuf.buf = reinterpret_cast<CHAR *>(c_socket);
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, c_socket, ex_over->_send_buf, 0, addr_size + 16, addr_size + 16, 0, &ex_over->_over);
			break;
		}
		case OP_RECV: {
			if (0 == num_bytes) disconnect(key);
			int remain_data = num_bytes + clients[key]._prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key]._prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			clients[key].do_recv();
			break;
		}
		case OP_SEND:
			if (0 == num_bytes) disconnect(key);
			delete ex_over;
			break;
		}
	}
}

int main()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);
	int client_id = 0;

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), g_h_iocp, 9999, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	OVER_EXP a_over;
	a_over._comp_type = OP_ACCEPT;
	a_over._wsabuf.buf = reinterpret_cast<CHAR *>(c_socket);
	AcceptEx(g_s_socket, c_socket, a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &a_over._over);

	vector <thread> worker_threads;
	for (int i = 0; i < 6; ++i)
		worker_threads.emplace_back(do_worker);
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);
	WSACleanup();
}
