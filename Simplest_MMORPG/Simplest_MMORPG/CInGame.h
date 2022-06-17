#pragma once

#ifndef __INGAME_H__
#define __INGAME_H__

#include "CScene.h"
class CInGame : public CScene
{
public:
	CInGame();
	virtual ~CInGame();

	// CScene을(를) 통해 상속됨
	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Late_update() override;
	virtual void Render(HDC _DC) override;
	virtual void Release() override;
};

#endif // !__INGAME_H__