#include "stdafx.h"
#include "CObj.h"
#include "CPlayer.h"
#include "CObjManager.h"
#include "CBmpManager.h"
#include "CSceneManager.h"
#include "CKeyManager.h"

CPlayer::CPlayer()
{
}

CPlayer::~CPlayer()
{
	Release();
}

void CPlayer::Initialize()
{
	CBmpManager::Get_Instance()->Insert_Bmp(L"../Image/Player.bmp", L"Player");

	m_tInfo.fX = MAPSTARTX + (TILECX >> 1);
	m_tInfo.fY = MAPSTARTY + (TILECY >> 1);
	m_tInfo.iCX = 25;
	m_tInfo.iCY = 25;

	m_eState = OBJSTATE::IDLE;
	m_eCurDir = OBJDIR::IDLE;
}

int CPlayer::Update()
{
	Key_Check();

	Update_Rect();
	Frame_Move();

	return OBJ_NOEVENET;
}

void CPlayer::Late_Update()
{
}

void CPlayer::Render(HDC _DC)
{
	HDC hMemDC;
	switch (m_eState)
	{
	case OBJSTATE::IDLE:
		hMemDC = CBmpManager::Get_Instance()->Find_Image(L"Player");
		GdiTransparentBlt(_DC, m_tRect.left - 15, m_tRect.top - 30
			, 50, 60, hMemDC
			, m_tFrame.iFrameStart * 50, m_tFrame.iFrameScene * 60
			, 50, 60
			, RGB(100, 100, 100));
		break;
	
	}

	if (CKeyManager::Get_Instance()->Key_Pressing(VK_LCONTROL))
		Rectangle(_DC, m_tRect.left, m_tRect.top, m_tRect.right, m_tRect.bottom);
}

void CPlayer::Release()
{
}

void CPlayer::Key_Check()
{
	if (CKeyManager::Get_Instance()->Key_Pressing(VK_LEFT))
	{
		m_tInfo.fX -= m_tInfo.fSpeed;
		m_eCurDir = OBJDIR::LEFT;
	}
	else if (CKeyManager::Get_Instance()->Key_Pressing(VK_RIGHT))
	{
		m_tInfo.fX += m_tInfo.fSpeed;
		m_eCurDir = OBJDIR::RIGHT;
	}
	else if (CKeyManager::Get_Instance()->Key_Pressing(VK_UP))
	{
		m_tInfo.fY -= m_tInfo.fSpeed;
		m_eCurDir = OBJDIR::TOP;
	}
	else if (CKeyManager::Get_Instance()->Key_Pressing(VK_DOWN))
	{
		m_tInfo.fY += m_tInfo.fSpeed;
		m_eCurDir = OBJDIR::BOTTOM;
	}
	else
		m_eCurDir = OBJDIR::IDLE;
}
