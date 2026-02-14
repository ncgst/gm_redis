#include "redis_util.hpp"
#include "main.hpp"

namespace redis {
	void buildTable(GarrysMod::Lua::ILuaBase* LUA, const std::vector<cpp_redis::reply>& replies)
	{
		LUA->CreateTable();

		int i = 1;
		for (auto reply : replies)
		{
			LUA->PushNumber(i++);

			switch (reply.get_type())
			{
			case cpp_redis::reply::type::error:
			case cpp_redis::reply::type::bulk_string:
			case cpp_redis::reply::type::simple_string:
				LUA->PushString(reply.as_string().c_str());
				break;

			case cpp_redis::reply::type::integer:
				LUA->PushNumber(static_cast<double>(reply.as_integer()));
				break;

			case cpp_redis::reply::type::array:
				buildTable(LUA, reply.as_array());
				break;

			case cpp_redis::reply::type::null:
				LUA->PushNil();
				break;
			}

			LUA->SetTable(-3);
		}
	}
}
