#pragma once

#include <QtCore/QObject>
#include "LuaHelper.h"

struct LuaQObject {
    constexpr static const char* tname = "QObject";

    QObject* object;
    QMetaObject* metaObject;
    Ownership ownership;
};

static LuaQObject* check_qobject(lua_State* L, int idx) {
    void* ud = luaL_checkudata(L, idx, LuaQObject::tname);
    if (!ud) luaL_error(L, QString("expected %1").arg(LuaQObject::tname).toStdString().c_str());
    return static_cast<LuaQObject*>(ud);
}