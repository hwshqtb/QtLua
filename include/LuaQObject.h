#pragma once

#include <QtCore/QObject>
#include "LuaHelper.h"
#include "LuaQtMethod.h"

struct LuaQObject {
    constexpr static const char* tname = "QObject";

    QObject* object;
    Ownership ownership;

    /**
     *  lua metamethods
     */
    static int indexLua(lua_State* L);
    static int newindexLua(lua_State* L);
    static int gcLua(lua_State* L);
    static int tostringLua(lua_State* L);

    /**
     *  lua methods
     */
    static int connectLua(lua_State* L);

    /**
     *  register
     */
    static void registerMetaTable(lua_State* L);

    /**
     *  push
     */
    static void push(lua_State* L, const QObject* mo, const char* objectName = nullptr, Ownership ownership = Ownership::CppOwnership);
};
