#pragma once

#include <lua.hpp>
#include <QtCore/QMetaObject>
#include "header.h"

struct LuaQMetaObject {
    constexpr static const char* tname = "QMetaObject";

    QMetaObject* mo;
    OwnerShip ownership;
};

/**
 *  Lua metamethods for QMetaObject userdata
 */
static int lua_qmetaobject_call(lua_State* L) {
    lua_newtable(L);
    return 1;
}

static int lua_qmetaobject_index(lua_State* L) {
}

static int lua_qmetaobject_newindex(lua_State* L) {
}

static int lua_qmetaobject_gc(lua_State* L) {
}

static int lua_qmetaobject_pairs(lua_State* L) {
}

static void registerQMetaObjectType(lua_State* L) {
    luaL_newmetatable(L, LuaQMetaObject::tname);

    lua_pushcfunction(L, lua_qmetaobject_call);
    lua_setfield(L, -2, "__call");

    lua_pushcfunction(L, lua_qmetaobject_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, lua_qmetaobject_newindex);
    lua_setfield(L, -2, "__newindex");

    lua_pushstring(L, "QMetaObject");
    lua_setfield(L, -2, "__metatable");

    lua_pushcfunction(L, lua_qmetaobject_pairs);
    lua_setfield(L, -2, "__pairs");

    lua_pushcfunction(L, lua_qmetaobject_gc);
    lua_setfield(L, -2, "__gc");

    lua_pop(L, 1);
}

/**
 *  Lua methods for QMetaObject userdata
 */
static LuaQMetaObject* check_qmetaobject(lua_State* L, int idx) {
    void* ud = luaL_checkudata(L, idx, LuaQMetaObject::tname);
    if (!ud) luaL_error(L, QString("expected %1").arg(LuaQMetaObject::tname).toStdString().c_str());
    return static_cast<LuaQMetaObject*>(ud);
}

static int lua_qml_create(lua_State* L) {
    const char* typeName = luaL_checkstring(L, 1);
    auto mo = g_dynamicMetaObjects.value(typeName, nullptr);
    if (!mo) {
        return luaL_error(L, "type '%s' is not registered", typeName);
    }

    QObject* obj = mo->newInstance();
    LuaQObject* ud = static_cast<LuaQObject*>(lua_newuserdata(L, sizeof(LuaQObject)));
    ud->object = obj;
    ud->ownership = OwnerShip::LuaOwnership;
    luaL_getmetatable(L, "QObject");
    lua_setmetatable(L, -2);

    // 为 QObject userdata 创建一个 uservalue 表，用于存放 Lua 侧动态字段（如信号回调）
    lua_newtable(L);
    lua_setuservalue(L, -2);

    return 1;
}

static int lua_qmetaobject_new(lua_State* L) {
    LuaQMetaObject* metaObj = check_qmetaobject(L, 1);
    QObject* obj = metaObj->mo->newInstance();
    LuaQObject* ud = static_cast<LuaQObject*>(lua_newuserdata(L, sizeof(LuaQObject)));
    ud->object = obj;
    ud->ownership = OwnerShip::LuaOwnership;
    luaL_getmetatable(L, "QObject");
    lua_setmetatable(L, -2);
    return 1;
}

void registerQtType(lua_State* L, const QMetaObject* mo, const char* typeName = nullptr) {
    if (!typeName) {
        typeName = mo->className();
    }

    lua_getglobal(L, "qt");
    lua_pushstring(L, typeName);
    lua_newtable(L);
    lua_pushcfunction(L, lua_qmetaobject_new);
    lua_setfield(L, -2, "new");
    luaL_setmetatable(L, LuaQMetaObject::tname);
    lua_settable(L, -3);
    lua_pop(L, 1);
}
