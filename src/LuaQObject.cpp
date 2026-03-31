#include <LuaQObject.h>
#include <QtCore/QMetaMethod>
#include <QtCore/QVariant>
#include <QLuaMethod.h>

int LuaQObject::indexLua(lua_State* L) {
    LuaQObject* obj = LuaHelper::check_proxy<LuaQObject>(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // check key defined in lua
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1);
    lua_pop(L, 1);

    // Check if it's a special method "connect"
    if (strcmp(key, "connect") == 0) {
        lua_pushcfunction(L, connectLua);
        return 1;
    }
    // Check if it's a property
    if (obj->object->metaObject()->indexOfProperty(key) >= 0) {
        QVariant value = obj->object->property(key);
        LuaHelper::lua_push_qvariant(L, value);
        return 1;
    }
    // Check if it's a method
    int methodCount = obj->object->metaObject()->methodCount();
    for (int i = 0; i < methodCount; ++i) {
        QMetaMethod method = obj->object->metaObject()->method(i);
        if (QString(method.name()) == key) {
            LuaQtMethod::push(L, obj->object, key);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

int LuaQObject::newindexLua(lua_State* L) {
    LuaQObject* obj = LuaHelper::check_proxy<LuaQObject>(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // in C++
    if (obj->object->metaObject()->indexOfProperty(key) >= 0) {
        QVariant value = LuaHelper::lua_to_qvariant(L, 3);
        obj->object->setProperty(key, value);
        return 0;
    }

    // in lua
    lua_getuservalue(L, 1);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, -3);
    // lua_setuservalue(L, 1);

    return 0;
}

int LuaQObject::gcLua(lua_State* L) {
    LuaQObject* obj = LuaHelper::check_proxy<LuaQObject>(L, 1);
    if (obj) {
        if (obj->ownership == Ownership::LuaOwnership) {
            obj->object->deleteLater();
        }
    }
    return 0;
}

int LuaQObject::tostringLua(lua_State* L) {
    LuaQObject* obj = LuaHelper::check_proxy<LuaQObject>(L, 1);
    lua_pushfstring(L, "<QObject: %p>", obj->object);
    return 1;
}

int LuaQObject::connectLua(lua_State* L) {
    LuaQObject* obj = LuaHelper::check_proxy<LuaQObject>(L, 1);
    LuaQtMethod* signalSig;
    signalSig = LuaHelper::check_proxy<LuaQtMethod>(L, 2);
    if (lua_isfunction(L, 3)) {
        lua_pushvalue(L, 3);
        int fnRef = luaL_ref(L, LUA_REGISTRYINDEX);

        int signalIndex = obj->object->metaObject()->indexOfSignal(signalSig->methodName);
        if (signalIndex < 0) {
            // fallback: match by name for overloaded signal variants
            int mcount = obj->object->metaObject()->methodCount();
            for (int i = 0; i < mcount; ++i) {
                QMetaMethod m = obj->object->metaObject()->method(i);
                if (m.methodType() == QMetaMethod::Signal && QString(m.name()) == QString(signalSig->methodName)) {
                    signalIndex = i;
                    break;
                }
            }
        }
        if (signalIndex < 0) {
            return luaL_error(L, "signal '%s' not found", signalSig->methodName);
        }
        QMetaMethod signalMethod = obj->object->metaObject()->method(signalIndex);

        QLuaMethod* proxy = new QLuaMethod(LuaFunction{ L, fnRef }, signalMethod, obj->object);

        int slotIndex = QObject::staticMetaObject.methodCount();
        if (!QMetaObject::connect(obj->object, signalIndex, proxy, slotIndex)) {
            luaL_error(L, "failed to connect signal");
            return 0;
        }

        lua_pushboolean(L, true);
    }
    else if (lua_isuserdata(L, 3)) {
    }
    return 1;
}

void LuaQObject::registerMetaTable(lua_State* L) {
    luaL_newmetatable(L, "QObject");

    lua_pushcfunction(L, gcLua);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, tostringLua);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, newindexLua);
    lua_setfield(L, -2, "__newindex");

    lua_pushcfunction(L, indexLua);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);
}

void LuaQObject::push(lua_State* L, const QObject* mo, const char* objectName, Ownership ownership) {
}
