#include "stdafx.h"
#include "CObjManager.h"
#include "CObj.h"

CObjManager* CObjManager::m_pInstance = nullptr;

CObjManager::CObjManager()
{
}

CObjManager::~CObjManager()
{
	Release();
}

void CObjManager::Update()
{
	for (int i = 0; i < OBJID::END; ++i)
	{
		for (auto iter = m_listObj[i].begin(); iter != m_listObj[i].end();)
		{
			int iEvent = (*iter)->Update();
			if (iEvent == OBJ_DEAD)
			{
				SAFE_DELETE(*iter);
				iter = m_listObj[i].erase(iter);
			}
			else
				++iter;
		}
	}
}

void CObjManager::Late_Update()
{
	for (int i = 0; i < OBJID::END; ++i)
	{
		for (auto& pObj : m_listObj[i])
			pObj->Late_Update();
	}
}

void CObjManager::Render(HDC _DC)
{
	for (int i = 0; i < OBJID::END; ++i)
	{
		for (auto& pObj : m_listObj[i])
		{
			if (!pObj)
				continue;
			pObj->Render(_DC);
		}
	}
}

void CObjManager::Release()
{
	for (int i = 0; i < OBJID::END; ++i)
	{
		for_each(m_listObj[i].begin(), m_listObj[i].end(), Safe_Delete<CObj*>);
		m_listObj[i].clear();
	}
}
