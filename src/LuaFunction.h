#pragma once

#include "LuaHelper.h"
#include <QtCore/QVariant>
#include <lua.hpp>
#include <QtCore/QDebug>
#include <QtCore/QVariantList>

struct LuaFunction {
    lua_State* L;
    int ref;

    QVariantList call(const QVariantList& args, int nresults = 0) const {
        if (ref == LUA_NOREF) {
            return {};
        }
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        for (const QVariant& arg : args) {
            lua_push_qvariant(L, arg);
        }
        if (lua_pcall(L, args.size(), nresults, 0) != LUA_OK) {
            const char* errorMsg = lua_tostring(L, -1);
            qWarning() << "Error calling Lua function:" << errorMsg;
            lua_pop(L, 1);
            return {};
        }
        QVariantList results;
        for (int i = 0; i < nresults; ++i) {
            results.append(lua_to_qvariant(L, -nresults + i));
        }
        lua_pop(L, nresults);
        return results;
    }
};

Q_DECLARE_METATYPE(LuaFunction)
