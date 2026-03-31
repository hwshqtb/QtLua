#include <LuaQMetaObject.h>
#include <LuaHelper.h>
#include <QtCore/QMetaMethod>
#include <QtCore/private/qmetaobjectbuilder_p.h>

int LuaQMetaObject::callLua(lua_State* L) {
    LuaQMetaObject* metaObj = LuaHelper::check_proxy<LuaQMetaObject>(L, 1);
    if (lua_type(L, 2) != LUA_TTABLE) {
        return luaL_error(L, "expected table");
    }
    QMetaObjectBuilder builder;
    // className
    lua_getfield(L, 2, "className");
    if (lua_isstring(L, -1)) {
        builder.setClassName(lua_tostring(L, -1));
    }
    lua_pop(L, 1);
    // signals
    lua_getfield(L, 2, "signals");
    if (lua_istable(L, -1)) {
        int signalCount = lua_rawlen(L, -1);
        for (int index = 1; index <= signalCount; ++index) {
            lua_rawgeti(L, -1, index);
            if (lua_isstring(L, -1)) {
                builder.addSignal(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    // slots
    lua_getfield(L, 2, "slots");
    if (lua_istable(L, -1)) {
        lua_pushnil(L); // 第一个键
        while (lua_next(L, -2) != 0) {
            if (lua_isstring(L, -2) && lua_isfunction(L, -1)) {
                const char* slotName = lua_tostring(L, -2);
                builder.addSlot(slotName);

                // 将函数存储起来，以便后续调用
                // 这里需要将函数存储到uservalue中
                lua_pushvalue(L, -1);
                lua_setfield(L, 1, slotName);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    // properties
    lua_getfield(L, 2, "properties");
    if (lua_istable(L, -1)) {
    }
    return 0;
}

int LuaQMetaObject::indexLua(lua_State* L) {
    LuaQMetaObject* metaObj = LuaHelper::check_proxy<LuaQMetaObject>(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // check key defined in lua
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    while (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_getfield(L, -1, "super");
        lua_remove(L, -2);
        if (luaL_testudata(L, -1, LuaQMetaObject::tname)) {
            lua_getuservalue(L, -1);
            lua_remove(L, -2);
            lua_pushvalue(L, 2);
            lua_rawget(L, -2);
        }
        else {
            lua_pop(L, 1);
            lua_pushnil(L);
            break;
        }
    }
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1);

    // check key defined in C++
    const QMetaObject* mo = metaObj->mo;
    for (int index = 0; index < mo->methodCount(); ++index) {
        QMetaMethod method = mo->method(index);
        if (method.name() == key) {
            LuaQtMethod* methodUd = static_cast<LuaQtMethod*>(lua_newuserdata(L, sizeof(LuaQtMethod)));
            methodUd->methodName = method.name();
            luaL_setmetatable(L, LuaQtMethod::tname);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

int LuaQMetaObject::newindexLua(lua_State* L) {
    LuaQMetaObject* metaObj = LuaHelper::check_proxy<LuaQMetaObject>(L, 1);
    const char* key = luaL_checkstring(L, 2);
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, -3);
    return 0;
}

int LuaQMetaObject::pairsLua(lua_State* L) {
    return 0;
}

int LuaQMetaObject::gcLua(lua_State* L) {
    LuaQMetaObject* metaObj = LuaHelper::check_proxy<LuaQMetaObject>(L, 1);
    if (metaObj->ownership == Ownership::LuaOwnership) {
        free(const_cast<QMetaObject*>(metaObj->mo));
    }
    return 0;
}

int LuaQMetaObject::tostringLua(lua_State* L) {
    LuaQMetaObject* metaObj = LuaHelper::check_proxy<LuaQMetaObject>(L, 1);
    lua_pushfstring(L, "<QMetaObject: %s>", metaObj->mo->className());
    return 1;
}

int LuaQMetaObject::newLua(lua_State* L) {
    LuaQMetaObject* metaObj = LuaHelper::check_proxy<LuaQMetaObject>(L, 1);
    QObject* obj = metaObj->mo->newInstance();
    LuaQObject* ud = static_cast<LuaQObject*>(lua_newuserdata(L, sizeof(LuaQObject)));
    ud->object = obj;
    ud->ownership = Ownership::LuaOwnership;
    luaL_setmetatable(L, "QObject");
    lua_newtable(L);
    lua_setuservalue(L, -2);
    return 1;
}

int LuaQMetaObject::singletonLua(lua_State* L) {
    return callLua(L);
}

void LuaQMetaObject::registerMetaTable(lua_State* L) {
    luaL_newmetatable(L, tname);

    lua_pushcfunction(L, callLua);
    lua_setfield(L, -2, "__call");

    lua_pushcfunction(L, indexLua);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, newindexLua);
    lua_setfield(L, -2, "__newindex");

    lua_pushstring(L, "QMetaObject");
    lua_setfield(L, -2, "__metatable");

    // lua_pushcfunction(L, lua_qmetaobject_pairs);
    // lua_setfield(L, -2, "__pairs");

    lua_pushcfunction(L, gcLua);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, tostringLua);
    lua_setfield(L, -2, "__tostring");

    lua_pop(L, 1);
}

void LuaQMetaObject::push(lua_State* L, const QMetaObject* mo, const char* typeName, Ownership ownership) {
    if (!typeName) {
        typeName = mo->className();
    }
    lua_getglobal(L, "qt");
    LuaQMetaObject* metaObj = static_cast<LuaQMetaObject*>(lua_newuserdata(L, sizeof(LuaQMetaObject)));
    metaObj->mo = mo;
    metaObj->ownership = ownership;
    lua_newtable(L);

    lua_pushcfunction(L, newLua);
    lua_setfield(L, -2, "new");

    lua_pushcfunction(L, singletonLua);
    lua_setfield(L, -2, "singleton");

    if (mo->superClass()) {
        lua_getglobal(L, mo->superClass()->className());
        if (luaL_testudata(L, -1, LuaQMetaObject::tname)) {
            lua_setfield(L, -2, "super");
        }
        else {
            lua_pop(L, 1);
            lua_getglobal(L, "qt");
            lua_getfield(L, -1, mo->superClass()->className());
            if (luaL_testudata(L, -1, LuaQMetaObject::tname)) {
                lua_setfield(L, -3, "super");
            }
            else {
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }

    lua_setuservalue(L, -2);
    luaL_setmetatable(L, LuaQMetaObject::tname);
    lua_setfield(L, -2, typeName);
    lua_pop(L, 1);
}
