#pragma once
#ifndef __STRUCT_H__
#define __STRUCT_H__

typedef struct tagInfo
{
	float		fX;
	float		fY;
	int			iCX;
	int			iCY;

	int			iHP;

	float		fSpeed;
	bool		bisDead;
}INFO;

typedef struct tagFrame
{
	int		iFrameStart;
	int		iFrameEnd;
	int		iFrameScene;
	DWORD	dwFrameTime;
	DWORD	dwFrameSpeed;
}FRAME;

#endif // !__STRUCT_H__
