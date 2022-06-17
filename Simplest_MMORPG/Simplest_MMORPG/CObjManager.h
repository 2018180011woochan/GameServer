#pragma once
#ifndef __OBJMANAGER_H__
#define __OBJMANAGER_H__

class CObj;
class CObjManager
{
public:
	CObjManager();
	~CObjManager();

public:
	void Update();
	void Late_Update();
	void Render(HDC _DC);
	void Release();

public:
	void Add_Object(CObj* _pObj, OBJID::ID _eID) { m_listObj[_eID].emplace_back(_pObj); }

public:
	static CObjManager* Get_Instance()
	{
		if (!m_pInstance)
			m_pInstance = new CObjManager;
		return m_pInstance;
	}
	static void Destroy_Instance()
	{
		SAFE_DELETE(m_pInstance);
	}

private:
	list<CObj*>				m_listObj[OBJID::END];

	static CObjManager*		m_pInstance;

};
#endif // !__OBJMANAGER_H__

