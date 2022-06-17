#include "stdafx.h"
#include "CInGame.h"
#include "CScene.h"
#include "CBmpManager.h"
#include "CSceneManager.h"
#include "CPlayer.h"
#include "CObjManager.h"

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

	CObj* pObj = nullptr;

	pObj = CAbstractFactory<CPlayer>::Create();
	CObjManager::Get_Instance()->Add_Object(pObj, OBJID::PLAYER);

}

void CInGame::Update()
{
	CObjManager::Get_Instance()->Update();
}

void CInGame::Late_update()
{
	CObjManager::Get_Instance()->Late_Update();
}

void CInGame::Render(HDC _DC)
{
	HDC hMemDC = CBmpManager::Get_Instance()->Find_Image(L"Stage");
	BitBlt(_DC, 0, 0, WINCX, WINCY, hMemDC, 0, 0, SRCCOPY);

	CObjManager::Get_Instance()->Render(_DC);
}

void CInGame::Release()
{
}
