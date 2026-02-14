#pragma once
#include <cpp_redis/cpp_redis>
#include <GarrysMod/Lua/Interface.h>

namespace redis {
	void buildTable(GarrysMod::Lua::ILuaBase* LUA, const std::vector<cpp_redis::reply>& replies);
}
