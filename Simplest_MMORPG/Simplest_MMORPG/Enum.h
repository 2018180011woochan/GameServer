#pragma once
#ifndef __ENUM_H__
#define __ENUM_H__

namespace OBJID
{
	enum ID {PLAYER, BOMB, BOSSBOMB, BOMBWAVE, TILE, BLOCK, MONSTER, BOSS, END};
}

namespace OBJDIR
{
	enum DIR { IDLE, TOP, BOTTOM, LEFT, RIGHT, TOPEND, BOTTOMEND, LEFTEND, RIGHTEND, END };
}

namespace OBJSTATE
{
	enum STATE { IDLE, WALK, BUBBLE, POP, ATTACK, HIT, DEAD, END };
}

#endif // !__ENUM_H__
