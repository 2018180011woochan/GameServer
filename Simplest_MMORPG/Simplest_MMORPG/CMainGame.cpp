#include "stdafx.h"
#include "CMainGame.h"
#include "CBmpManager.h"
#include "CSceneManager.h"
#include "CObjManager.h"

CMainGame::CMainGame()
{
}

CMainGame::~CMainGame()
{
	Release();
}

void CMainGame::Initialize()
{
	CBmpManager::Get_Instance()->Insert_Bmp(L"../Image/stage1.bmp", L"Stage1Back");
	CBmpManager::Get_Instance()->Insert_Bmp(L"../Image/BackBuffer.bmp", L"BackBuffer");

	CSceneManager::Get_Instance()->Scene_Change(CSceneManager::SCENE_INGAME);

	m_DC = GetDC(g_hWnd);
}

void CMainGame::Update()
{
	CSceneManager::Get_Instance()->Update();
}

void CMainGame::Late_Update()
{
	CSceneManager::Get_Instance()->Late_Update();
}

void CMainGame::Render()
{
	HDC HMemDC = CBmpManager::Get_Instance()->Find_Image(L"Stage1Back");
	HDC HBackBuffer = CBmpManager::Get_Instance()->Find_Image(L"BackBuffer");

	BitBlt(HBackBuffer, 0, 0, WINCX, WINCY, HMemDC, 0, 0, SRCCOPY);

	CSceneManager::Get_Instance()->Render(HBackBuffer);

	BitBlt(m_DC, 0, 0, WINCX, WINCY, HBackBuffer, 0, 0, SRCCOPY);
}

void CMainGame::Release()
{
	CBmpManager::Destroy_Instance();
	CSceneManager::Destroy_Instance();
	CObjManager::Destroy_Instance();
	ReleaseDC(g_hWnd, m_DC);
}
