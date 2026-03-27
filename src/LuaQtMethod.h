#pragma once

#include <lua.hpp>
#include "LuaHelper.h"
#include <QtCore/QObject>
#include <QtCore/QMetaObject>

struct LuaQtMethod {
    constexpr static const char* tname = "QtMethod";

    QObject* object;
    QMetaObject* metaObject;
    const char* methodName;
};

static LuaQtMethod* check_qmethod(lua_State* L, int idx) {
    void* ud = luaL_checkudata(L, idx, LuaQtMethod::tname);
    if (!ud) luaL_error(L, QString("expected %1").arg(LuaQtMethod::tname).toStdString().c_str());
    return static_cast<LuaQtMethod*>(ud);
}

static int lua_qtmethod_caller(lua_State* L) {
    // L[1] = methodName, L[2] = obj, L[3+] = args
    LuaQObject* obj = check_qobject(L, 2);
    const char* methodName = check_qmethod(L, 1);

    int argCount = lua_gettop(L) - 2;
    const QMetaObject* mo = obj->object->metaObject();
    QMetaMethod matchedMethod;
    QGenericArgument argArray[10];
    LuaQtArgStorage argStorage[10];
    bool found = false;

    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod method = mo->method(i);
        if (QString(method.name()) != QString(methodName)) continue;
        if (method.parameterCount() != argCount) continue;

        QGenericArgument testArgs[10];
        LuaQtArgStorage testStorage[10];
        if (!prepare_method_arguments(L, 3, method, testArgs, testStorage)) {
            continue;
        }

        matchedMethod = method;
        for (int j = 0; j < 10; ++j) {
            argArray[j] = testArgs[j];
            argStorage[j] = testStorage[j];
        }
        found = true;
        break;
    }

    if (!found) {
        return luaL_error(L, "method '%s' with %d args not found", methodName, argCount);
    }

    bool ok = false;
    QVariant result;

    int retType = matchedMethod.returnType();
    LuaQtArgStorage returnStorage;
    QGenericReturnArgument returnArg;

    if (retType != QMetaType::Void) {
        switch (retType) {
            case QMetaType::Int:
                returnStorage.intValue = 0;
                returnArg = QGenericReturnArgument("int", &returnStorage.intValue);
                break;
            case QMetaType::Bool:
                returnStorage.boolValue = false;
                returnArg = QGenericReturnArgument("bool", &returnStorage.boolValue);
                break;
            case QMetaType::Double:
                returnStorage.doubleValue = 0.0;
                returnArg = QGenericReturnArgument("double", &returnStorage.doubleValue);
                break;
            case QMetaType::QString:
                returnStorage.stringValue.clear();
                returnArg = QGenericReturnArgument("QString", &returnStorage.stringValue);
                break;
            case QMetaType::QVariant:
                returnStorage.variantValue.clear();
                returnArg = QGenericReturnArgument("QVariant", &returnStorage.variantValue);
                break;
            default:
                return luaL_error(L, "unsupported return type for method '%s'", methodName);
        }

        ok = matchedMethod.invoke(obj->object, Qt::AutoConnection,
            returnArg,
            argArray[0], argArray[1], argArray[2], argArray[3], argArray[4],
            argArray[5], argArray[6], argArray[7], argArray[8], argArray[9]);
    }
    else {
        ok = matchedMethod.invoke(obj->object, Qt::AutoConnection,
            argArray[0], argArray[1], argArray[2], argArray[3], argArray[4],
            argArray[5], argArray[6], argArray[7], argArray[8], argArray[9]);
    }

    if (!ok) {
        return luaL_error(L, "failed to invoke method '%s'", methodName);
    }

    if (retType != QMetaType::Void) {
        result = convert_return_value(retType, returnStorage);
        if (result.isValid()) {
            lua_push_qvariant(L, result);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int lua_qtmethod_tostring(lua_State* L) {
    LuaQtMethod* method = check_qmethod(L, 1);
    lua_pushfstring(L, "<QtMethod: %s>", method->methodName);
    return 1;
}

static void registerQtMethodType(lua_State* L) {
    luaL_newmetatable(L, "QtMethod");

    lua_pushcfunction(L, lua_qtmethod_caller);
    lua_setfield(L, -2, "__call");

    lua_pushcfunction(L, lua_qtmethod_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pop(L, 1);
}
