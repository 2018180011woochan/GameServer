#pragma once
class CMyBitMap;
class CBmpManager
{
public:
	CBmpManager();
	~CBmpManager();

public:
	void Insert_Bmp(const TCHAR* _pFileName, const TCHAR* _pImageKey);
	HDC Find_Image(const TCHAR* _pImagekey);
	void Release();

public:
	static CBmpManager* Get_Instance()
	{
		if (!m_pInstance)
			m_pInstance = new CBmpManager;
		return m_pInstance;
	}
	static void Destroy_Instance()
	{
		SAFE_DELETE(m_pInstance);
	}

private:
	static CBmpManager* m_pInstance;
	map<const TCHAR*, CMyBitMap*>	m_mapBit;
};
