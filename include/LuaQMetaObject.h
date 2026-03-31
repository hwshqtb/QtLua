#pragma once

#include <lua.hpp>
#include <QtCore/QMetaObject>
#include <LuaHelper.h>
#include "LuaQObject.h"

struct LuaQMetaObject {
    constexpr static const char* tname = "QMetaObject";

    const QMetaObject* mo;
    Ownership ownership;

    /**
     *  lua metamethods
     */
    static int callLua(lua_State* L);
    static int indexLua(lua_State* L);
    static int newindexLua(lua_State* L);
    static int pairsLua(lua_State* L);
    static int gcLua(lua_State* L);
    static int tostringLua(lua_State* L);

    /**
     *  lua methods
     */
    static int newLua(lua_State* L);
    static int singletonLua(lua_State* L);

    /**
     *  register
     */
    static void registerMetaTable(lua_State* L);

    /**
     *  push
     */
    static void push(lua_State* L, const QMetaObject* mo, const char* typeName = nullptr, Ownership ownership = Ownership::CppOwnership);
};