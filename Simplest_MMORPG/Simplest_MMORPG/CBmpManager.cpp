#include "stdafx.h"
#include "CBmpManager.h"
#include "CMyBitMap.h"

CBmpManager* CBmpManager::m_pInstance = nullptr;

CBmpManager::CBmpManager()
{
}


CBmpManager::~CBmpManager()
{
	Release();
}

void CBmpManager::Insert_Bmp(const TCHAR* _pFileName, const TCHAR* _pImageKey)
{
	auto iter = find_if(m_mapBit.begin(), m_mapBit.end(), CStrCmp(_pImageKey));

	if (m_mapBit.end() == iter)
	{
		CMyBitMap* pBmp = new CMyBitMap;
		pBmp->Load_Bmp(_pFileName);

		m_mapBit.emplace(_pImageKey, pBmp);
	}
}

HDC CBmpManager::Find_Image(const TCHAR* _pImagekey)
{
	auto iter = find_if(m_mapBit.begin(), m_mapBit.end(), CStrCmp(_pImagekey));

	if (m_mapBit.end() == iter)
		return nullptr;
	return iter->second->Get_MemDC();
}

void CBmpManager::Release()
{
	for (auto& iter : m_mapBit)
	{
		if (nullptr != iter.second)
			Safe_Delete(iter.second);
	}
	m_mapBit.clear();
}