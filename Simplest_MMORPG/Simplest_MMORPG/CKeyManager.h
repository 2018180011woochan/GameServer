#pragma once
#ifndef __KEYMANAGER_H__
#define __KEYMANAGER_H__

#define VK_MAX 0xff

class CKeyManager
{
public:
	CKeyManager();
	~CKeyManager();

public:
	bool Key_Pressing(int _key);
	bool Key_Down(int _key);
	void Key_Update();

public:
	static CKeyManager* Get_Instance()
	{
		if (!m_pInstance)
			m_pInstance = new CKeyManager;
		return m_pInstance;
	}
	static void Destroy_Instance()
	{
		SAFE_DELETE(m_pInstance);
	}

private:
	static CKeyManager* m_pInstance;
	bool					m_bKeyState[VK_MAX];
};


#endif // !__KEYMANAGER_H__
