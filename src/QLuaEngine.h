#pragma once

#include "LuaHelper.h"
#include "LuaQMetaObject.h"
#include "LuaQObject.h"
#include "LuaQtMethod.h"
#include "LuaFunction.h"

#include "QLuaValue.h"
#include "QLuaMethod.h"

static QVariant lua_to_qvariant(lua_State* L, int idx) {
    if (lua_isnumber(L, idx)) {
        return QVariant((double)lua_tonumber(L, idx));
    }
    if (lua_isinteger(L, idx)) {
        return QVariant((int)lua_tointeger(L, idx));
    }
    if (lua_isstring(L, idx)) {
        return QVariant(QString::fromUtf8(lua_tostring(L, idx)));
    }
    if (lua_isboolean(L, idx)) {
        return QVariant((bool)lua_toboolean(L, idx));
    }
    if (lua_isfunction(L, idx)) {
        lua_pushvalue(L, idx);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return QVariant::fromValue(LuaFunction{ L, ref });
    }
    return QVariant();
}

static void lua_push_qvariant(lua_State* L, const QVariant& value) {
    if (value.typeName() == QMetaType::typeName(QMetaType::Int)) {
        lua_pushinteger(L, value.toInt());
    }
    else if (value.typeName() == QMetaType::typeName(QMetaType::Double)) {
        lua_pushnumber(L, value.toDouble());
    }
    else if (value.typeName() == QMetaType::typeName(QMetaType::QString)) {
        lua_pushstring(L, value.toString().toUtf8().constData());
    }
    else if (value.typeName() == QMetaType::typeName(QMetaType::Bool)) {
        lua_pushboolean(L, value.toBool());
    }
    else if (value.typeName() == "LuaFunction") {
        LuaFunction func = value.value<LuaFunction>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, func.ref);
    }
    else {
        lua_pushnil(L);
    }
}
