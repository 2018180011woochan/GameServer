#pragma once
#ifndef __DEFINE_H__
#define __DEFINE_H__

#define WINCX 800
#define WINCY 600

#define SAFE_DELETE(p) if (p) { delete p; p = nullptr; }

#define OBJ_DEAD 1
#define OBJ_NOEVENET 0

#define MAPSTARTX 20
#define MAPSTARTY 40

#define TILECX 40
#define TILECY 40

#define TILEX 15
#define TILEY 13




#endif // !__DEFINE_H__
