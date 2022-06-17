#pragma once
#ifndef __MYBITMAP_H__
#define __MYBITMAP_H__

class CMyBitMap
{
public:
	CMyBitMap();
	~CMyBitMap();

public:
	HDC Get_MemDC() { return m_hMemDC; }
	void Load_Bmp(const TCHAR* _pFileName);
	void Release();

private:
	HDC			m_hMemDC;
	HBITMAP		m_hBitMap;
	HBITMAP		m_hOldBmp;
};


#endif // !__MYBITMAP_H__
