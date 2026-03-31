#include <LuaQtMethod.h>
#include <QtCore/QMetaMethod>
#include <QtCore/QVariant>
#include <LuaQObject.h>
#include <LuaHelper.h>

// in lua obj:f(...)
// but considered as obj1.f(obj2, ...)
// like method.invoke(obj2, ...) where method from obj1's meta object
int LuaQtMethod::callLua(lua_State* L) {
    LuaQObject* obj = LuaHelper::check_proxy<LuaQObject>(L, 2);
    LuaQtMethod* method = LuaHelper::check_proxy<LuaQtMethod>(L, 1);

    int argCount = lua_gettop(L) - 2;
    const QMetaObject* mo = method->object->metaObject();
    QMetaMethod matchedMethod;
    QGenericArgument argArray[10];
    QVariant argStorage[10];

    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod metaMethod = mo->method(i);
        if (QString(metaMethod.name()) != QString(method->methodName)) continue;
        if (metaMethod.parameterCount() != argCount) continue;
        matchedMethod = metaMethod;
        for (int j = 0; j < argCount; ++j) {
            argArray[j] = LuaHelper::lua_to_qgenericargument(L, 3 + j, argStorage[j], metaMethod.parameterMetaType(j));
            if (argArray[j].data() == nullptr) {
                return luaL_error(L, "method '%s' with %d args not found", method->methodName, argCount);
            }
        }
        for (int j = argCount; j < 10; ++j) {
            argArray[j] = QGenericArgument();
        }
        break;
    }

    bool ok = false;
    QGenericReturnArgument returnArg;
    QVariant returnStorage;
    returnArg = LuaHelper::lua_to_qgenericreturnargument(L, 1, returnStorage, matchedMethod.returnMetaType());
    ok = matchedMethod.invoke(obj->object, Qt::AutoConnection,
            returnArg,
            argArray[0], argArray[1], argArray[2], argArray[3], argArray[4],
            argArray[5], argArray[6], argArray[7], argArray[8], argArray[9]);

    if (!ok) {
        return luaL_error(L, "failed to invoke method '%s'", method->methodName);
    }

    LuaHelper::lua_push_qvariant(L, returnStorage);
    return 1;
}

int LuaQtMethod::tostringLua(lua_State* L) {
    LuaQtMethod* method = LuaHelper::check_proxy<LuaQtMethod>(L, 1);
    lua_pushfstring(L, "<QtMethod: %s>", method->methodName);
    return 1;
}

int LuaQtMethod::gcLua(lua_State* L) {
    LuaQtMethod* method = LuaHelper::check_proxy<LuaQtMethod>(L, 1);
    delete[] const_cast<char*>(method->methodName);
    return 0;
}

void LuaQtMethod::registerMetaTable(lua_State* L) {
    luaL_newmetatable(L, "QtMethod");

    lua_pushcfunction(L, callLua);
    lua_setfield(L, -2, "__call");

    lua_pushcfunction(L, tostringLua);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, gcLua);
    lua_setfield(L, -2, "__gc");

    lua_pop(L, 1);
}

void LuaQtMethod::push(lua_State* L, QObject* obj, const char* methodName) {
    LuaQtMethod* method = static_cast<LuaQtMethod*>(lua_newuserdata(L, sizeof(LuaQtMethod)));
    method->object = obj;
    std::size_t nameLen = strlen(methodName);
    method->methodName = new char[strlen(methodName) + 1];
    strcpy(const_cast<char*>(method->methodName), methodName);
    const_cast<char*>(method->methodName)[nameLen] = '\0';

    luaL_getmetatable(L, "QtMethod");
    lua_setmetatable(L, -2);
}
