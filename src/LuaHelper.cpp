#include <LuaHelper.h>
#include <QtCore/QVariant>
#include <lua.hpp>
#include <QtCore/QDebug>
#include <QtCore/QVariantList>
#include <LuaFunction.h>

QVariant LuaHelper::lua_to_qvariant(lua_State* L, int idx) {
    if (lua_isnumber(L, idx)) {
        return QVariant((double)lua_tonumber(L, idx));
    }
    if (lua_isinteger(L, idx)) {
        return QVariant((int)lua_tointeger(L, idx));
    }
    if (lua_isstring(L, idx)) {
        return QVariant(QString::fromUtf8(lua_tostring(L, idx)));
    }
    if (lua_isboolean(L, idx)) {
        return QVariant((bool)lua_toboolean(L, idx));
    }
    if (lua_isfunction(L, idx)) {
        lua_pushvalue(L, idx);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return QVariant::fromValue(LuaFunction{ L, ref });
    }
    return QVariant();
}

void LuaHelper::lua_push_qvariant(lua_State* L, const QVariant& value) {
    if (value.metaType() == QMetaType(QMetaType::Int)) {
        lua_pushinteger(L, value.toInt());
    }
    else if (value.metaType() == QMetaType(QMetaType::Double)) {
        lua_pushnumber(L, value.toDouble());
    }
    else if (value.metaType() == QMetaType(QMetaType::QString)) {
        lua_pushstring(L, value.toString().toUtf8().constData());
    }
    else if (value.metaType() == QMetaType(QMetaType::Bool)) {
        lua_pushboolean(L, value.toBool());
    }
    else if (value.typeName() == "LuaFunction") {
        LuaFunction func = value.value<LuaFunction>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, func.ref);
    }
    else {
        lua_pushnil(L);
    }
}

QGenericArgument LuaHelper::lua_to_qgenericargument(lua_State* L, int idx, QVariant& storage, QMetaType type) {
    storage = lua_to_qvariant(L, idx);
    if (type.id() != QMetaType::UnknownType) {
        if (storage.typeId() != type.id()) {
            if (storage.canConvert(type.id())) {
                storage.convert(type.id());
            }
            else {
                qWarning() << "Cannot convert Lua value to required type:" << type.name();
                return QGenericArgument();
            }
        }
    }
    return QGenericArgument(storage.typeName(), storage.data());
}

QGenericReturnArgument LuaHelper::lua_to_qgenericreturnargument(lua_State* L, int idx, QVariant& storage, QMetaType type) {
    storage = QVariant::fromMetaType(type);
    if (storage.isValid())
        return QGenericReturnArgument(storage.typeName(), storage.data());
    return QGenericReturnArgument();
}
