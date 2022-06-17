#pragma once

#ifndef __SCENEMANAGER_H__
#define __SCENEMANAGER_H__

class CScene;
class CSceneManager
{
public:
	CSceneManager();
	~CSceneManager();

public:
	enum SCENEID {
		SCENE_INGAME, SCENE_END
	};

public:
	void Scene_Change(SCENEID _eScene);
	void Update();
	void Late_Update();
	void Render(HDC _DC);
	void Release();


public:
	static CSceneManager* Get_Instance()
	{
		if (!m_pInstance)
			m_pInstance = new CSceneManager;
		return m_pInstance;
	}
	static void Destroy_Instance()
	{
		SAFE_DELETE(m_pInstance);
	}


private:
	static CSceneManager* m_pInstance;
	CScene*				m_pScene;
	SCENEID				m_ePreScene;
	SCENEID				m_eCurScene;
};


#endif // !__SCENEMANAGER_H__
