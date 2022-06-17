#pragma once
#ifndef __OBJ_H__
#define __OBJ_H__

class CObj
{
public:
	CObj();
	virtual ~CObj();

public:
	virtual void Initialize() = 0;
	virtual int Update() = 0;
	virtual void Late_Update() = 0;
	virtual void Render(HDC _DC) = 0;
	virtual void Release() = 0;

protected:
	void Update_Rect();
	void Frame_Move();

public:
	const INFO& Get_Info() const { return m_tInfo; }
	const RECT& Get_Rect() const { return m_tRect; }

public:
	OBJSTATE::STATE GetState() { return m_eState; }

protected:
	INFO				m_tInfo;
	RECT				m_tRect;
	OBJSTATE::STATE		m_eState;
	OBJDIR::DIR			m_ePreDir;
	OBJDIR::DIR			m_eCurDir;

	FRAME				m_tFrame;

	DWORD				m_dwTime;
};


#endif // !__OBJ_H__
