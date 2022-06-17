#pragma once

#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "CObj.h"
class CPlayer : public CObj
{
public:
	CPlayer();
	virtual ~CPlayer();

	// CObj을(를) 통해 상속됨
	virtual void Initialize() override;
	virtual int Update() override;
	virtual void Late_Update() override;
	virtual void Render(HDC _DC) override;
	virtual void Release() override;

private:
	void Key_Check();
};

#endif // !__PLAYER_H__