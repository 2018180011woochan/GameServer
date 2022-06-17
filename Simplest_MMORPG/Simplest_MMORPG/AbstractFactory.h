#pragma once

#ifndef __ABSTRACTFACTORY_H__
#define __ABSTRACTFACTORY_H__

//class CObj;
//class CStageMap;
//class CTile;
//
template<typename T>
class CAbstractFactory
{
//public:
//	static CObj* Create()
//	{
//		CObj* pObj = new T;
//		pObj->Initialize();
//
//		return pObj;
//	}
//
//	static CObj* Create(float _x, float _y)
//	{
//		CObj* pObj = new T;
//		pObj->Initialize();
//		pObj->Set_Pos(_x, _y);
//
//		return pObj;
//	}
//
//	static CObj* Create(float _x, float _y, int _TileKey)
//	{
//		CObj* pObj = new T;
//		pObj->Initialize();
//		pObj->Set_Pos(_x, _y);
//		dynamic_cast<CTile*>(pObj)->Set_TileKey(_TileKey);
//
//		return pObj;
//	}
//
//	static CObj* Create(float _x, float _y, OBJDIR::DIR _Dir, int BombPower)
//	{
//		CObj* pObj = new T;
//		pObj->Initialize();
//		pObj->Set_Pos(_x, _y);
//		dynamic_cast<CBombWave*>(pObj)->Set_WaveDir(_Dir);
//		dynamic_cast<CBombWave*>(pObj)->SetBombPower(BombPower);
//		return pObj;
//	}
//
//	static CObj* Create(float _x, float _y, OBJDIR::DIR _Dir)
//	{
//		CObj* pObj = new T;
//		pObj->Initialize();
//		pObj->Set_Pos(_x, _y);
//		dynamic_cast<CBombWave*>(pObj)->Set_WaveDir(_Dir);
//
//		return pObj;
//	}
//
//	static CObj* Create_Monster(float _x, float _y, OBJDIR::DIR _Dir)
//	{
//		CObj* pObj = new T;
//		pObj->Initialize();
//		pObj->Set_Pos(_x, _y);
//	    pObj->SetCurDIR(_Dir);
//
//		return pObj;
//	}
//
//	static CObj* Create(float _x, float _y, int _bombPower, bool _iscollid)
//	{
//		CObj* pObj = new T;
//		pObj->Initialize();
//		pObj->Set_Pos(_x, _y);
//		pObj->SetBombPower(_bombPower);
//	
//		return pObj;
//	}


};

#endif // !__ABSTRACTFACTORY_H__
