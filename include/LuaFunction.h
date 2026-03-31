#pragma once

#include "LuaHelper.h"
#include <QtCore/QVariant>
#include <lua.hpp>
#include <QtCore/QVariantList>

struct LuaFunction {
    lua_State* L;
    int ref;

    QVariantList call(const QVariantList& args, int nresults = 0) const;
};

Q_DECLARE_METATYPE(LuaFunction)
