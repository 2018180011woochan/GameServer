#include "stdafx.h"
#include "CInGame.h"
#include "CScene.h"
#include "CBmpManager.h"

CInGame::CInGame()
{
}

CInGame::~CInGame()
{
	Release();
}

void CInGame::Initialize()
{
	//¹è°æ
	CBmpManager::Get_Instance()->Insert_Bmp(L"../Image/test.bmp", L"Stage");
}

void CInGame::Update()
{
}

void CInGame::Late_update()
{
}

void CInGame::Render(HDC _DC)
{
	HDC hMemDC = CBmpManager::Get_Instance()->Find_Image(L"Stage");
	BitBlt(_DC, 0, 0, WINCX, WINCY, hMemDC, 0, 0, SRCCOPY);
}

void CInGame::Release()
{
}
