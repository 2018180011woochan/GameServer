#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>

#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <string>

#include <windows.h>  
#include <locale.h>

#define UNICODE  
#include <sqlext.h>  

extern "C" {
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}

#pragma comment (lib, "lua54.lib")

#include "protocol.h"
#include "Enum.h"

constexpr int RANGE = 11;

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;

//////////////// DB //////////////
SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt = 0;
SQLRETURN retcode;

SQLINTEGER user_id, user_xpos, user_ypos;
SQLLEN cbuser_id = 0, cbuser_xpos = 0, cbuser_ypos = 0;
/// //////////////////////////////

bool isAllowAccess(int clientid, int cid);
void SavePos(int clientid, int cid);

enum EVENT_TYPE { EV_MOVE, EV_HEAL, EV_ATTACK };
struct TIMER_EVENT {
	int object_id;
	EVENT_TYPE ev;
	chrono::system_clock::time_point act_time;
	int target_id;

	constexpr bool operator < (const TIMER_EVENT& _Left) const
	{
		return (act_time > _Left.act_time);
	}

};

priority_queue<TIMER_EVENT> timer_queue;
mutex timer_l;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_PLAYER_MOVE };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	int target_id;
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
	//int _clientid;
	SOCKET _socket;
	int		_id;
	short	x, y;
	char	_name[NAME_SIZE];
	short	race;
	short	level;
	int		exp, maxexp;
	int		hp, hpmax;
	short	attack_type;
	short	move_type;
	unordered_set <int> view_list;
	mutex	vl;
	lua_State* L;
	mutex	vm_l;
	mutex	run_l;
	bool isNpcRun;
	bool isNpcDead;

	chrono::system_clock::time_point next_move_time;
	int		_prev_remain;
public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = rand() % W_WIDTH;
		y = rand() % W_HEIGHT;
		_name[0] = 0;
		race = RACE::RACE_END;
		level = 1;
		exp = 0;
		maxexp = level * 100;
		hpmax = level * 100;
		hp = hpmax;
		attack_type = ATTACKTYPE::ATTACKTYPE_END;
		move_type = MOVETYPE::MOVETYPE_END;
		_s_state = ST_FREE;
		_prev_remain = 0;
		next_move_time = chrono::system_clock::now() + chrono::seconds(1);
		isNpcRun = false;
		isNpcDead = false;
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
	void send_login_ok_packet()
	{
		SC_LOGIN_OK_PACKET p;
		p.size = sizeof(SC_LOGIN_OK_PACKET);
		p.type = SC_LOGIN_OK;
		///////////////DB에 있다면 DB에서 꺼내올 것들 //////////////////
		p.id = _id;
		p.x = x;
		p.y = y;
		p.level = level;
		p.exp = exp;
		p.hpmax = hpmax;
		p.hp = p.hpmax;
		////////////////////////////////////////////////////////////////
		do_send(&p);
	}
	void send_move_packet(int c_id, int client_time);
	void send_add_object(int c_id);
	void send_remove_object(int c_id);
	void send_chat_packet(int c_id, const char* mess);
};

array<SESSION, MAX_USER + NUM_NPC> clients;
HANDLE g_h_iocp;
SOCKET g_s_socket;

void add_timer(int obj_id, int act_time, EVENT_TYPE e_type, int target_id)
{
	using namespace chrono;
	TIMER_EVENT ev;
	ev.act_time = system_clock::now() + milliseconds(act_time);
	ev.object_id = obj_id;
	ev.ev = e_type;
	ev.target_id = target_id;
	timer_queue.push(ev);
}

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

void SESSION::send_add_object(int c_id)
{
	SC_ADD_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_ADD_OBJECT_PACKET);
	p.type = SC_ADD_OBJECT;
	p.x = clients[c_id].x;
	p.y = clients[c_id].y;
	p.race = clients[c_id].race;
	p.level = clients[c_id].level;
	p.hp = clients[c_id].hp;
	p.hpmax = clients[c_id].hpmax;
	strcpy_s(p.name, clients[c_id]._name);
	do_send(&p);
}

void SESSION::send_remove_object(int c_id)
{
	SC_REMOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	p.type = SC_REMOVE_OBJECT;
	do_send(&p);
}

void SESSION::send_chat_packet(int c_id, const char *mess)
{
	SC_CHAT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_CHAT_PACKET) - sizeof(p.mess) + strlen(mess) + 1;
	p.type = SC_CHAT;
	strcpy_s(p.mess, mess);
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



void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

		//if (!isAllowAccess(p->client_id, c_id))
		//{
		//	clients[c_id]._clientid = p->client_id;
		//	clients[c_id].send_login_info_packet(false);
		//	break;
		//}

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
		clients[c_id]._id = p->id;
		clients[c_id].send_login_ok_packet();
		clients[c_id]._s_state = ST_INGAME;
		clients[c_id]._sl.unlock();

		//clients[c_id].x = rand() % W_WIDTH;
		//clients[c_id].y = rand() % W_HEIGHT;

		clients[c_id].x = 0;
		clients[c_id].y = 0;


		for (int i = 0; i < MAX_USER; ++i) {
			auto& pl = clients[i];
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
				pl.send_add_object(c_id);
			}
			pl._sl.unlock();
		}
		for (auto& pl : clients) {
			if (pl._id == c_id) continue;
			lock_guard<mutex> aa{ pl._sl };
			if (ST_INGAME != pl._s_state) continue;

			if (RANGE >= distance(pl._id, c_id)) {
				clients[c_id].vl.lock();
				clients[c_id].view_list.insert(pl._id);
				clients[c_id].vl.unlock();
				clients[c_id].send_add_object(pl._id);
			}
		}
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		short x = clients[c_id].x;
		short y = clients[c_id].y;

		unordered_set<int> old_vl;
		for (int i = MAX_USER; i < NUM_NPC; ++i) {
			if (clients[i]._s_state != ST_INGAME) continue;
			if (distance(c_id, i) <= RANGE) old_vl.insert(i);
		}

		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}
		clients[c_id].x = x;
		clients[c_id].y = y;
		for (int i = 0; i < MAX_USER; ++i) {
			auto& pl = clients[i];
			lock_guard<mutex> aa{ pl._sl };
			if (ST_INGAME == pl._s_state)
				pl.send_move_packet(c_id, p->client_time);
		}
		///////////////////// NPC 시야 ///////////////////////////////// 
		unordered_set<int> new_vl;
		for (int i = MAX_USER; i < NUM_NPC; ++i) {
			if (clients[i]._s_state != ST_INGAME) continue;
			if (distance(c_id, i) <= RANGE) new_vl.insert(i);
		}

		for (auto p_id : new_vl) {
			if (0 == clients[c_id].view_list.count(p_id)) {
				clients[c_id].view_list.insert(p_id);
				clients[c_id].send_add_object(p_id);
			}
			else {
				clients[c_id].send_move_packet(p_id, 0);
			}
		}
		for (auto p_id : old_vl) {
			if (0 == new_vl.count(p_id)) {
				if (clients[c_id].view_list.count(p_id) == 1) {
					clients[c_id].view_list.erase(p_id);

					clients[c_id].send_remove_object(p_id);
				}
			}
		}
		////////////////////////////////////////////////////////////////////

		for (int i = 0; i < NUM_NPC; ++i) {
			int npc_id = MAX_USER + i;
			if (distance(npc_id, c_id) < RANGE) {
				auto ex_over = new OVER_EXP;
				ex_over->_comp_type = OP_PLAYER_MOVE;
				ex_over->target_id = c_id;
				PostQueuedCompletionStatus(g_h_iocp, 1, npc_id, &ex_over->_over);
			}
		}

		//SavePos(clients[c_id]._clientid, c_id);

		break;
	}
	case CS_ATTACK: {
		for (int i = MAX_USER; i < NUM_NPC; ++i) {
			if (clients[i].isNpcDead)
				continue;
			if (clients[c_id].x == clients[i].x) {
				if (abs(clients[c_id].y - clients[i].y) == 1) {	// 상하
					clients[i].hp -= clients[c_id].level * 50;
					SC_STAT_CHANGE_PACKET Monsterscp;
					Monsterscp.size = sizeof(SC_STAT_CHANGE_PACKET);
					Monsterscp.type = SC_STAT_CHANGE;
					Monsterscp.hp = clients[i].hp;
					Monsterscp.hpmax = clients[i].hpmax;
					Monsterscp.id = i;
					Monsterscp.level = clients[i].level;

					clients[c_id].do_send(&Monsterscp);

					cout << clients[c_id]._name << "가 " << clients[i]._name << "["
						<< clients[i]._id << "] " << "를 공격하여 " << clients[c_id].level * 50
						<< "의 데미지를 입혔습니다\n";
					if (clients[i].hp <= 0) {					// 처치한다면
						SC_REMOVE_OBJECT_PACKET p;
						p.size = sizeof(SC_REMOVE_OBJECT_PACKET);
						p.type = SC_REMOVE_OBJECT;
						p.id = i;
						clients[c_id].do_send(&p);				// 나중에는 접속한 모든 플레이어한테 보내야할듯

						SC_STAT_CHANGE_PACKET scp;
						scp.size = sizeof(SC_STAT_CHANGE_PACKET);
						scp.type = SC_STAT_CHANGE;
						scp.id = c_id;
						int rewardEXP = int(clients[i].level * clients[i].level) * 2;

						clients[c_id].exp += rewardEXP;
						if (clients[c_id].exp > clients[c_id].maxexp)		// 렙업
						{
							clients[c_id].level += 1;
							clients[c_id].hpmax = clients[c_id].level * 100;
							clients[c_id].hp = clients[c_id].hpmax;
							clients[c_id].maxexp = clients[c_id].level * 100;
							clients[c_id].exp = 0;

							cout << clients[c_id]._name << "의 레벨이 " << clients[c_id].level
								<< "가 되었습니다\n";
						}
						scp.level = clients[c_id].level;
						scp.hp = clients[c_id].hp;
						scp.hpmax = clients[c_id].hpmax;
						scp.exp = clients[c_id].exp;

						clients[c_id].do_send(&scp);
						cout << clients[c_id]._name << "가 " << clients[i]._name << "["
							<< clients[i]._id << "] " << "를 무찔러 " << rewardEXP
							<< "의 경험치를 얻었습니다\n";
						clients[i].isNpcDead = true;
					}
				}
			}
			if (clients[c_id].y == clients[i].y) {
				if (abs(clients[c_id].x - clients[i].x) == 1) {	// 좌우
					clients[i].hp -= clients[c_id].level * 50;
					SC_STAT_CHANGE_PACKET Monsterscp;
					Monsterscp.size = sizeof(SC_STAT_CHANGE_PACKET);
					Monsterscp.type = SC_STAT_CHANGE;
					Monsterscp.hp = clients[i].hp;
					Monsterscp.hpmax = clients[i].hpmax;
					Monsterscp.id = i;
					Monsterscp.level = clients[i].level;

					clients[c_id].do_send(&Monsterscp);

					cout << clients[c_id]._name << "가 " << clients[i]._name << "["
						<< clients[i]._id << "] " << "를 공격하여 " << clients[c_id].level * 50
						<< "의 데미지를 입혔습니다\n";
					if (clients[i].hp <= 0) {					// 처치한다면
						SC_REMOVE_OBJECT_PACKET p;
						p.size = sizeof(SC_REMOVE_OBJECT_PACKET);
						p.type = SC_REMOVE_OBJECT;
						p.id = i;
						clients[c_id].do_send(&p);				// 나중에는 접속한 모든 플레이어한테 보내야할듯

						SC_STAT_CHANGE_PACKET scp;
						scp.size = sizeof(SC_STAT_CHANGE_PACKET);
						scp.type = SC_STAT_CHANGE;
						scp.id = c_id;
						int rewardEXP = int(clients[i].level * clients[i].level) * 2;

						clients[c_id].exp += rewardEXP;
						if (clients[c_id].exp > clients[c_id].maxexp)		// 렙업
						{
							clients[c_id].level += 1;
							clients[c_id].hpmax = clients[c_id].level * 100;
							clients[c_id].hp = clients[c_id].hpmax;
							clients[c_id].maxexp = clients[c_id].level * 100;
							clients[c_id].exp = 0;

							cout << clients[c_id]._name << "의 레벨이 " << clients[c_id].level
								<< "가 되었습니다\n";
						}
						scp.level = clients[c_id].level;
						scp.hp = clients[c_id].hp;
						scp.hpmax = clients[c_id].hpmax;
						scp.exp = clients[c_id].exp;

						clients[c_id].do_send(&scp);
						cout << clients[c_id]._name << "가 " << clients[i]._name << "["
							<< clients[i]._id << "] " << "를 무찔러 " << rewardEXP
							<< "의 경험치를 얻었습니다\n";
						clients[i].isNpcDead = true;
					}
				}
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
		int client_id = static_cast<int>(key);
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
			ex_over->_wsabuf.buf = reinterpret_cast<CHAR*>(c_socket);
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, c_socket, ex_over->_send_buf, 0, addr_size + 16, addr_size + 16, 0, &ex_over->_over);
			break;
		}
		case OP_RECV: {
			if (0 == num_bytes) disconnect(client_id);
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
			if (0 == num_bytes) disconnect(client_id);
			delete ex_over;
			break;
		case OP_PLAYER_MOVE: {
			//auto L = clients[client_id].L;
			//clients[client_id].vm_l.lock();
			//lua_getglobal(L, "event_player_move");
			//lua_pushnumber(L, ex_over->target_id);
			//lua_pcall(L, 1, 0, 0);
			//clients[client_id].vm_l.unlock();
			delete ex_over;
		}
						   break;
		} //SWITCH
	} // WHILE
}

void move_npc(int npc_id)
{
	short x = clients[npc_id].x;
	short y = clients[npc_id].y;
	unordered_set<int> old_vl;
	for (int i = 0; i < MAX_USER; ++i) {
		if (clients[i]._s_state != ST_INGAME) continue;
		if (distance(npc_id, i) <= RANGE) old_vl.insert(i);
	}
	switch (rand() % 4) {
	case 0: if (y > 0) y--; break;
	case 1: if (y < W_HEIGHT - 1) y++; break;
	case 2: if (x > 0) x--; break;
	case 3: if (x < W_WIDTH - 1) x++; break;
	}

	clients[npc_id].x = x;
	clients[npc_id].y = y;

	unordered_set<int> new_vl;
	for (int i = 0; i < MAX_USER; ++i) {
		if (clients[i]._s_state != ST_INGAME) continue;
		if (distance(npc_id, i) <= RANGE) new_vl.insert(i);
	}

	for (auto p_id : new_vl) {
			clients[p_id].vl.lock();
			if (0 == clients[p_id].view_list.count(npc_id)) {
				clients[p_id].view_list.insert(npc_id);
				clients[p_id].vl.unlock();
				clients[p_id].send_add_object(npc_id);
			}
			else {
				clients[p_id].vl.unlock();
				clients[p_id].send_move_packet(npc_id, 0);
			}
	}
	for (auto p_id : old_vl) {
		if (0 == new_vl.count(p_id)) {
			clients[p_id].vl.lock();
			if (clients[p_id].view_list.count(npc_id) == 1) {
				clients[p_id].view_list.erase(npc_id);
				clients[p_id].vl.unlock();
				clients[p_id].send_remove_object(npc_id);
			}
			else
				clients[p_id].vl.unlock();
		}
	}
}

void AttackNPC(int npc_id)
{
	for (int i = 0; i < MAX_USER; ++i) {
		if (clients[i]._s_state != ST_INGAME) continue;
		if (clients[npc_id].y == clients[i].y) {
			if (abs(clients[npc_id].x - clients[i].x) == 1) {	// 좌우
				int AttackPower = clients[npc_id].level * 10;
				clients[i].hp -= AttackPower;

				SC_STAT_CHANGE_PACKET scp;
				scp.size = sizeof(SC_STAT_CHANGE_PACKET);
				scp.type = SC_STAT_CHANGE;
				scp.id = i;
				scp.hp = clients[i].hp;
				scp.hpmax = clients[i].hpmax;
				scp.exp = clients[i].exp;
				scp.level = clients[i].level;

				cout << clients[npc_id]._name << "[" << clients[i]._id << "] " << "의 공격으로 "
					<< clients[i]._name << "의 HP가 " << clients[i].hp << "가 되었습니다.\n";

				clients[i].do_send(&scp);
			}
		}

		if (clients[npc_id].x == clients[i].x) {
			if (abs(clients[npc_id].y - clients[i].y) == 1) {	// 상하
				int AttackPower = clients[npc_id].level * 10;
				clients[i].hp -= AttackPower;

				SC_STAT_CHANGE_PACKET scp;
				scp.size = sizeof(SC_STAT_CHANGE_PACKET);
				scp.type = SC_STAT_CHANGE;
				scp.id = i;
				scp.hp = clients[i].hp;
				scp.hpmax = clients[i].hpmax;
				scp.exp = clients[i].exp;
				scp.level = clients[i].level;

				cout << clients[npc_id]._name << "[" << clients[i]._id << "] " << "의 공격으로 "
					<< clients[i]._name << "의 HP가 " << clients[i].hp << "가 되었습니다.\n";

				clients[i].do_send(&scp);
			}
		}
	}
}

void do_ai_ver_1()
{
	for (;;) {
		auto start_t = chrono::system_clock::now();
		for (int i = 0; i < NUM_NPC; ++i) {
			int npc_id = i + MAX_USER;
			if (start_t > clients[npc_id].next_move_time) {
				move_npc(npc_id);
				//AttackNPC(npc_id);
				clients[npc_id].next_move_time = start_t + chrono::seconds(1);
			}
		}
	}
}

void do_ai_ver_heat_beat()
{
	for (;;) {
		auto start_t = chrono::system_clock::now();
		for (int i = 0; i < NUM_NPC; ++i) {
			int npc_id = i + MAX_USER;
			//if (!clients[npc_id].isNpcRun)
			move_npc(npc_id);
		}

		auto end_t = chrono::system_clock::now();
		auto ai_t = end_t - start_t;
		cout << "AI time : " << chrono::duration_cast<chrono::milliseconds>(ai_t).count();
		cout << "ms\n";
		this_thread::sleep_until(start_t + chrono::seconds(1));
	}

}

void SavePos(int clientid, int cid)
{
	///// //////////////////////////////////////////////////////////////////////////////
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	wstring c_id = to_wstring(clientid);
	wstring xpos = to_wstring(clients[cid].x);
	wstring ypos = to_wstring(clients[cid].y);

	wstring storedProcedure = L"EXEC updatepos ";
	storedProcedure += c_id;
	storedProcedure += L", ";
	storedProcedure += xpos;
	storedProcedure += L", ";
	storedProcedure += ypos;

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)storedProcedure.c_str(), SQL_NTS);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

		retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &user_xpos, 10, &cbuser_xpos);
		retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &user_ypos, 10, &cbuser_ypos);

		for (int i = 0; ; i++) {
			retcode = SQLFetch(hstmt);
			break;
		}
	}
	///// ////////////////////////////////////////////////////////////////////////////////////////
}

bool isAllowAccess(int clientid, int cid)
{
	/// //////////////////////////////////////////////////////////////////////////////
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	wstring c_id = to_wstring(clientid);

	wstring storedProcedure = L"EXEC selectid ";
	storedProcedure += c_id;

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)storedProcedure.c_str(), SQL_NTS);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

		// Bind columns 1, 2, and 3  
		retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &user_id, 10, &cbuser_id);
		retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &user_xpos, 10, &cbuser_xpos);
		retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &user_ypos, 10, &cbuser_ypos);

		// Fetch and print each row of data. On an error, display a message and exit.  
		for (int i = 0; ; i++) {
			retcode = SQLFetch(hstmt);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				//printf("ID %5d : x : %5d, y : %5d\n", user_id, user_xpos, user_ypos);

				////// DB에서 위치 받아오기 //////////		
				clients[cid].x = user_xpos;
				clients[cid].y = user_ypos;
				return true;
			}
			else
			{
				std::cout << "그런 아이디는 없습니다\n";
				return false;
			}
		}
	}
	/// ////////////////////////////////////////////////////////////////////////////////////////
}

int API_SendMessage(lua_State* L)
{
	int client_id = lua_tonumber(L, -3);
	int npc_id = lua_tonumber(L, -2);
	const char* mess = lua_tostring(L, -1);
	lua_pop(L, 4);

	clients[client_id].send_chat_packet(npc_id, mess);
	
	// 메세지 보냈으면 도망 코드 실행

	clients[npc_id].isNpcRun = true;
	int randindex = rand() % 4;

	clients[client_id].vm_l.lock();
	lua_getglobal(L, "event_npc_run");
	lua_pushnumber(L, client_id);
	lua_pushnumber(L, randindex);
	lua_pushnumber(L, 0);		// 처음 run함수 실행시킬땐 runindex = 0
	lua_pcall(L, 3, 0, 0);
	clients[client_id].vm_l.unlock();
	
	return 0;
}

int API_Run(lua_State* L)
{
	int client_id = lua_tonumber(L, -5);
	int npc_id = lua_tonumber(L, -4);
	int new_x = lua_tonumber(L, -3);
	int new_y = lua_tonumber(L, -2);
	int runIndex = lua_tonumber(L, -1);
	lua_pop(L, 6);
	
	
	if (runIndex > 3)	// 세번 도망침
	{
		clients[npc_id].isNpcRun = false;
		clients[client_id].send_chat_packet(npc_id, "BYE");
	}
	else				// 세번 도망치기 전이라면 함수 다시 호출
	{
		int randindex = rand() % 4;

		short x = clients[npc_id].x;
		short y = clients[npc_id].y;
		unordered_set<int> old_vl;
		for (int i = 0; i < MAX_USER; ++i) {
			if (clients[i]._s_state != ST_INGAME) continue;
			if (distance(npc_id, i) <= RANGE) old_vl.insert(i);
		}
		clients[npc_id].run_l.lock();
		cout << "npc " << npc_id << " is running away ";
		cout << " x : " << new_x << " y: " << new_y << endl;
		clients[npc_id].x = new_x;
		clients[npc_id].y = new_y;
		clients[npc_id].run_l.unlock();

		unordered_set<int> new_vl;
		for (int i = 0; i < MAX_USER; ++i) {
			if (clients[i]._s_state != ST_INGAME) continue;
			if (distance(npc_id, i) <= RANGE) new_vl.insert(i);
		}

		for (auto p_id : new_vl) {
			clients[p_id].vl.lock();
			if (0 == clients[p_id].view_list.count(npc_id)) {
				clients[p_id].view_list.insert(npc_id);
				clients[p_id].vl.unlock();
				clients[p_id].send_add_object(npc_id);
			}
			else {
				clients[p_id].vl.unlock();
				clients[p_id].send_move_packet(npc_id, 0);
			}
		}
		for (auto p_id : old_vl) {
			if (0 == new_vl.count(p_id)) {
				clients[p_id].vl.lock();
				if (clients[p_id].view_list.count(npc_id) == 1) {
					clients[p_id].view_list.erase(npc_id);
					clients[p_id].vl.unlock();
					clients[p_id].send_remove_object(npc_id);
				}
				else
					clients[p_id].vl.unlock();
			}
		}

		auto start_t = chrono::system_clock::now();

		lua_getglobal(L, "event_npc_run");
		lua_pushnumber(L, client_id);
		lua_pushnumber(L, randindex);
		lua_pushnumber(L, runIndex);

		this_thread::sleep_until(start_t + chrono::seconds(1));

		lua_pcall(L, 3, 0, 0);
	}


	return 0;
}

int API_get_x(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);

	int x = clients[obj_id].x;

	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int obj_id = lua_tonumber(L, -1);
	lua_pop(L, 2);

	int y = clients[obj_id].y;

	lua_pushnumber(L, y);
	return 1;
}

void initialize_npc()
{
	for (int i = 0; i < NUM_NPC + MAX_USER; ++i)
		clients[i]._id = i;
	cout << "NPC initialize Begin.\n";
	for (int i = MAX_USER; i < MAX_USER + 50000 ; ++ i)
	{
		// Skeleton
		int npc_id = i;
		clients[npc_id]._s_state = ST_INGAME;
		clients[npc_id].race = RACE::RACE_SKELETON;
		clients[npc_id].level = 1;
		clients[npc_id].hpmax = clients[npc_id].level * 100;
		clients[npc_id].hp = clients[npc_id].hpmax;

		strcpy_s(clients[npc_id]._name, "Skeleton");
	}
	for (int i = MAX_USER + 50000; i < MAX_USER + 100000; ++i)
	{
		// Wraith
		int npc_id = i;
		clients[npc_id]._s_state = ST_INGAME;
		clients[npc_id].race = RACE::RACE_WRIATH;
		clients[npc_id].level = 2;
		clients[npc_id].hpmax = clients[npc_id].level * 100;
		clients[npc_id].hp = clients[npc_id].hpmax;

		strcpy_s(clients[npc_id]._name, "Wriath");
	}
	for (int i = MAX_USER + 100000; i < MAX_USER + 150000; ++i)
	{
		// Devil
		int npc_id = i;
		clients[npc_id]._s_state = ST_INGAME;
		clients[npc_id].race = RACE::RACE_DEVIL;
		clients[npc_id].level = 3;
		clients[npc_id].hpmax = clients[npc_id].level * 100;
		clients[npc_id].hp = clients[npc_id].hpmax;

		strcpy_s(clients[npc_id]._name, "Devil");
	}
	for (int i = MAX_USER + 150000; i < NUM_NPC; ++i)
	{
		// Diablo
		int npc_id = i;
		clients[npc_id]._s_state = ST_INGAME;
		clients[npc_id].race = RACE::RACE_DIABLO;
		clients[npc_id].level = 4;
		clients[npc_id].hpmax = clients[npc_id].level * 100;
		clients[npc_id].hp = clients[npc_id].hpmax;

		strcpy_s(clients[npc_id]._name, "Diablo");
	}
	//for (int i = 0; i < NUM_NPC; ++i) {
	//	int npc_id = i + MAX_USER;
	//	clients[npc_id]._s_state = ST_INGAME;
	//	
	//	strcpy_s(clients[npc_id]._name, "Wriath");
	//	//sprintf_s(clients[npc_id]._name, "M-%d", npc_id);
	//	/*lua_State* L = luaL_newstate();
	//	clients[npc_id].L = L;

	//	luaL_openlibs(L);
	//	luaL_loadfile(L, "hello.lua");
	//	lua_pcall(L, 0, 0, 0);

	//	lua_getglobal(L, "set_object_id");
	//	lua_pushnumber(L, npc_id);
	//	lua_pcall(L, 1, 0, 0);

	//	lua_register(L, "API_chat", API_SendMessage);
	//	lua_register(L, "API_get_x", API_get_x);
	//	lua_register(L, "API_get_y", API_get_y);
	//	lua_register(L, "API_run", API_Run);*/
	//}
	cout << "NPC Initialization complete.\n";
}

void do_timer()
{

}

void initialize_DB()
{
	setlocale(LC_ALL, "korean");

	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
	SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

	retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2018180011_KWC", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	std::cout << "DB Access OK\n";
}

int main()
{
	initialize_npc();
	//initialize_DB();

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

	thread ai_thread{ do_ai_ver_1 };
	ai_thread.join();
	
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);
	WSACleanup();
}
