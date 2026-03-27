#pragma once

#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/QVariant>
#include <lua.hpp>

enum struct Ownership {
    CppOwnership,
    LuaOwnership
};

struct LuaQObject;
struct LuaQMetaObject;
struct LuaQMetaMethod;
struct LuaFunction;

static QVariant lua_to_qvariant(lua_State* L, int idx);

static void lua_push_qvariant(lua_State* L, const QVariant& value);

template <class LuaProxy>
LuaProxy* check_proxy(lua_State* L, int idx) {
    void* ud = luaL_checkudata(L, idx, LuaProxy::tname);
    if (!ud) luaL_error(L, QString("expected %1").arg(LuaProxy::tname).toStdString().c_str());
    return static_cast<LuaProxy*>(ud);
}