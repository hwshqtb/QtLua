#pragma once

#include <lua.hpp>
#include "LuaHelper.h"
#include <QtCore/QObject>
#include <QtCore/QMetaObject>

struct LuaQtMethod {
    constexpr static const char* tname = "QtMethod";

    QObject* object;
    const char* methodName;

    /**
     *  lua metamethods
     */
    static int callLua(lua_State* L);
    static int tostringLua(lua_State* L);
    static int gcLua(lua_State* L);

    /**
     *  lua methods
     */

    /**
     *  register
     */
    static void registerMetaTable(lua_State* L);

    /**
     *  push
     */
    static void push(lua_State* L, QObject* obj, const char* methodName);
};