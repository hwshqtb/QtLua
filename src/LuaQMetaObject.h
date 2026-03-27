#pragma once

#include <lua.hpp>
#include <QtCore/QMetaObject>
#include "LuaHelper.h"
#include "LuaQObject.h"

struct LuaQMetaObject {
    constexpr static const char* tname = "QMetaObject";

    const QMetaObject* mo;
    Ownership ownership;
};

static LuaQMetaObject* check_qmetaobject(lua_State* L, int idx) {
    void* ud = luaL_checkudata(L, idx, LuaQMetaObject::tname);
    if (!ud) luaL_error(L, QString("expected %1").arg(LuaQMetaObject::tname).toStdString().c_str());
    return static_cast<LuaQMetaObject*>(ud);
}

/**
 *  Lua metamethods for QMetaObject userdata
 */
static int lua_qmetaobject_call(lua_State* L) {
    lua_newtable(L);
    return 1;
}

static int lua_qmetaobject_index(lua_State* L) {
    LuaQMetaObject* metaObj = check_qmetaobject(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // check key defined in lua
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    while (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        if (lua_getfield(L, -1, "super") == LUA_TTABLE) {
            lua_getuservalue(L, -1);
            lua_pushvalue(L, 2);
            lua_rawget(L, -2);
        }
        else {
            lua_pop(L, 1);
            break;
        }
    }
    if (!lua_isnil(L, -1)) {
        return 1;
    }

    // check key defined in C++
    const QMetaObject* mo = metaObj->mo;
    int propIndex = mo->indexOfProperty(key);
    if (propIndex != -1) {
        lua_pushcfunction(L, lua_qproperty_index);
        lua_pushvalue(L, 1);
        lua_pushinteger(L, propIndex);
        return 3;
    }
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

static int lua_qmetaobject_new(lua_State* L) {
    LuaQMetaObject* metaObj = check_qmetaobject(L, 1);
    QObject* obj = metaObj->mo->newInstance();
    LuaQObject* ud = static_cast<LuaQObject*>(lua_newuserdata(L, sizeof(LuaQObject)));
    ud->object = obj;
    ud->ownership = Ownership::LuaOwnership;
    luaL_setmetatable(L, "QObject");
    lua_newtable(L);
    lua_setuservalue(L, -2);
    return 1;
}

void registerQtType(lua_State* L, const QMetaObject* mo, const char* typeName = nullptr) {
    if (!typeName) {
        typeName = mo->className();
    }
    lua_getglobal(L, "qt");
    LuaQMetaObject* metaObj = static_cast<LuaQMetaObject*>(lua_newuserdata(L, sizeof(LuaQMetaObject)));
    metaObj->mo = mo;
    metaObj->ownership = Ownership::CppOwnership;
    lua_pushcfunction(L, lua_qmetaobject_new);
    lua_setfield(L, -2, "new");
    luaL_setmetatable(L, LuaQMetaObject::tname);
    lua_newtable(L);
    lua_setuservalue(L, -2);

    lua_getglobal(L, mo->superClass()->className());
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "super");
    }
    else {
        lua_pop(L, 1);
        lua_getglobal(L, "qt");
        lua_getfield(L, -1, mo->superClass()->className());
        if (lua_istable(L, -1)) {
            lua_pushvalue(L, -2);
            lua_setfield(L, -2, "super");
        }
    }
    lua_pop(L, 1);

    lua_setfield(L, -2, typeName);
    lua_pop(L, 1);
}
