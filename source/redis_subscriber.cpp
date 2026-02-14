#include "main.hpp"
#include "redis_subscriber.h"
#include "redis_util.hpp"

void redis::subscriber::Initialize(GarrysMod::Lua::ILuaBase* LUA)
{
	BaseInterface::InitMetatable(LUA, "redis_subscriber");

	LUA->PushCFunction(wrap(lua_Ping));
	LUA->SetField(-2, "Ping");
	LUA->PushCFunction(wrap(lua_Auth));
	LUA->SetField(-2, "Auth");

	LUA->PushCFunction(wrap(lua_Subscribe));
	LUA->SetField(-2, "Subscribe");

	LUA->PushCFunction(wrap(lua_Unsubscribe));
	LUA->SetField(-2, "Unsubscribe");

	LUA->PushCFunction(wrap(lua_PSubscribe));
	LUA->SetField(-2, "PSubscribe");

	LUA->PushCFunction(wrap(lua_PUnsubscribe));
	LUA->SetField(-2, "PUnsubscribe");

	LUA->Pop();
}

void redis::subscriber::HandleAction(GarrysMod::Lua::ILuaBase* LUA, subAction action)
{
	if (action.type == redis::globals::actionType::Message)
	{
		LUA->ReferencePush(redis::globals::iRefDebugTraceBack);
		if (redis::PushCallback(LUA, m_refOnMessage, 1, "OnMessage"))
		{
			LUA->Push(1);
				LUA->PushString(action.data.channel.c_str());
				LUA->PushString(action.data.message.c_str());

				if (LUA->PCall(3, 0, -5) != 0)
					redis::ErrorNoHalt(LUA, "[redis OnMessage callback error] ");
			LUA->Pop();
		}
		else
			LUA->Pop();
	} else if (action.type == redis::globals::actionType::Reply) {
		if (action.data.reference > 0)
		{
			LUA->ReferencePush(redis::globals::iRefDebugTraceBack);
			LUA->ReferencePush(action.data.reference);
			LUA->Push(1);

			switch (action.data.reply.get_type())
			{
			case cpp_redis::reply::type::error:
			case cpp_redis::reply::type::bulk_string:
			case cpp_redis::reply::type::simple_string:
				LUA->PushString(action.data.reply.as_string().c_str());
				break;

			case cpp_redis::reply::type::integer:
				LUA->PushNumber(static_cast<double>(action.data.reply.as_integer()));
				break;

			case cpp_redis::reply::type::array:
				buildTable(LUA, action.data.reply.as_array());
				break;

			case cpp_redis::reply::type::null:
				LUA->PushNil();
				break;
			}

			if (LUA->PCall(2, 0, -4) != 0)
				redis::ErrorNoHalt(LUA, "[redis Send callback error] ");
			LUA->Pop();

			LUA->ReferenceFree(action.data.reference);
		}
	}
}

int redis::subscriber::GetCallbackOptional(GarrysMod::Lua::ILuaBase* LUA, int stackPos)
{
	if (LUA->Top() >= stackPos && !LUA->IsType(stackPos, GarrysMod::Lua::Type::NIL))
	{
		LUA->CheckType(stackPos, GarrysMod::Lua::Type::FUNCTION);
		LUA->Push(stackPos);

		return LUA->ReferenceCreate();
	}

	return GarrysMod::Lua::Type::NONE;
}

// https://redis.io/commands/ping/
int redis::subscriber::lua_Ping(GarrysMod::Lua::ILuaBase* LUA)
{
	subscriber* ptr = GetSubscriber(LUA, 1, true);

	try
	{
		ptr->m_iface.ping();
	}
	catch (const cpp_redis::redis_error& e)
	{
		LUA->PushNil();
		LUA->PushString(e.what());
		return 2;
	}

	LUA->PushBool(true);
	return 1;
}

// https://redis.io/commands/auth/
int redis::subscriber::lua_Auth(GarrysMod::Lua::ILuaBase* LUA)
{
	subscriber* ptr = GetSubscriber(LUA, 1, true);

	const char* password = LUA->CheckString(2);
	int callbackRef = GetCallbackOptional(LUA, 3);

	try
	{
		if (callbackRef == GarrysMod::Lua::Type::NONE)
			ptr->m_iface.auth(password);
			else
				ptr->m_iface.auth(password, [ptr, callbackRef](cpp_redis::reply& reply)
					{
						ptr->EnqueueAction({ redis::globals::actionType::Reply, subActionData(reply, callbackRef) });
					});
	}
	catch (const cpp_redis::redis_error& e)
	{
		LUA->PushNil();
		LUA->PushString(e.what());
		return 2;
	}

	LUA->PushBool(true);
	return 1;
}

// https://redis.io/commands/subscribe/
int redis::subscriber::lua_Subscribe(GarrysMod::Lua::ILuaBase* LUA)
{
	subscriber* ptr = GetSubscriber(LUA, 1, true);
	const char* channel = LUA->CheckString(2);

	try
	{
		ptr->m_iface.subscribe(channel, [ptr](const std::string& channel, const std::string& message)
			{
				ptr->EnqueueAction({ redis::globals::actionType::Message, subActionData(channel, message) });
			});
	}
	catch (const cpp_redis::redis_error& e)
	{
		LUA->PushNil();
		LUA->PushString(e.what());
		return 2;
	}

	LUA->PushBool(true);
	return 1;
}

// https://redis.io/commands/psubscribe/
int redis::subscriber::lua_PSubscribe(GarrysMod::Lua::ILuaBase* LUA)
{
	subscriber* ptr = GetSubscriber(LUA, 1, true);
	const char* channel = LUA->CheckString(2);

	try
	{
		ptr->m_iface.psubscribe(channel, [ptr](const std::string& channel, const std::string& message)
			{
				ptr->EnqueueAction({ redis::globals::actionType::Message, subActionData(channel, message) });
			});
	}
	catch (const cpp_redis::redis_error& e)
	{
		LUA->PushNil();
		LUA->PushString(e.what());
		return 2;
	}

	LUA->PushBool(true);
	return 1;
}

// https://redis.io/commands/unsubscribe/
int redis::subscriber::lua_Unsubscribe(GarrysMod::Lua::ILuaBase* LUA)
{
	subscriber* ptr = GetSubscriber(LUA, 1, true);
	const char* channel = LUA->CheckString(2);

	try
	{
		ptr->m_iface.unsubscribe(channel);
	}
	catch (const cpp_redis::redis_error& e)
	{
		LUA->PushNil();
		LUA->PushString(e.what());
		return 2;
	}

	LUA->PushBool(true);
	return 1;
}

// https://redis.io/commands/punsubscribe/
int redis::subscriber::lua_PUnsubscribe(GarrysMod::Lua::ILuaBase* LUA)
{
	subscriber* ptr = GetSubscriber(LUA, 1, true);
	const char* channel = LUA->CheckString(2);

	try
	{
		ptr->m_iface.punsubscribe(channel);
	}
	catch (const cpp_redis::redis_error& e)
	{
		LUA->PushNil();
		LUA->PushString(e.what());
		return 2;
	}

	LUA->PushBool(true);
	return 1;
}