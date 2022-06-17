#include "stdafx.h"
#include "CSceneManager.h"
#include "CInGame.h"

CSceneManager* CSceneManager::m_pInstance = nullptr;
CSceneManager::CSceneManager()
	:m_pScene(nullptr), m_eCurScene(SCENE_END), m_ePreScene(SCENE_END)
{
}


CSceneManager::~CSceneManager()
{
	Release();
}

void CSceneManager::Scene_Change(SCENEID _eScene)
{
	m_eCurScene = _eScene;
	if (m_ePreScene != m_eCurScene)
	{
		SAFE_DELETE(m_pScene);

		switch (m_eCurScene)
		{
		case CSceneManager::SCENE_INGAME:
			m_pScene = new CInGame;
			break;
		}
		m_pScene->Initialize();
		m_ePreScene = m_eCurScene;
	}
}

void CSceneManager::Update()
{
	m_pScene->Update();
}

void CSceneManager::Late_Update()
{
	m_pScene->Late_update();
}

void CSceneManager::Render(HDC _DC)
{
	m_pScene->Render(_DC);
}

void CSceneManager::Release()
{
	SAFE_DELETE(m_pScene);
}
