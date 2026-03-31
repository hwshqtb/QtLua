#pragma once

#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/QVariant>
#include <lua.hpp>
#include <QtCore/QDebug>
#include <LuaFunction.h>
#include <LuaQObject.h>
#include <LuaQMetaObject.h>

class QLuaValue {
public:
    QLuaValue(lua_State* L, int ref = LUA_NOREF):
        _L(L),
        _ref(ref) {}

    ~QLuaValue() {
        if (_ref != LUA_NOREF) {
            luaL_unref(_L, LUA_REGISTRYINDEX, _ref);
        }
    }

    bool isNil() const {
        if (_ref == LUA_NOREF) return true;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isnil(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isBoolean() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isboolean(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool toBoolean() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_toboolean(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isNumber() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isnumber(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    double toNumber() const {
        if (_ref == LUA_NOREF) return 0.0;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        double result = lua_tonumber(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isString() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isstring(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    QString toString() const {
        if (_ref == LUA_NOREF) return QString();
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        QString result = QString::fromUtf8(lua_tostring(_L, -1));
        lua_pop(_L, 1);
        return result;
    }

    bool isUserData() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isuserdata(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isQObject() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = luaL_checkudata(_L, -1, "QObject") != nullptr;
        lua_pop(_L, 1); // pop the original value
        return result;
    }

    QObject* toQObject() const {
        if (!isQObject()) return nullptr;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        LuaQObject* ud = static_cast<LuaQObject*>(luaL_checkudata(_L, -1, "QObject"));
        QObject* obj = ud->object;
        lua_pop(_L, 1);
        return obj;
    }

    bool isQMetaObject() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = luaL_checkudata(_L, -1, "QMetaObject") != nullptr;
        lua_pop(_L, 1); // pop the original value
        return result;
    }

    const QMetaObject* toQMetaObject() const {
        if (!isQMetaObject()) return nullptr;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        LuaQMetaObject* ud = static_cast<LuaQMetaObject*>(luaL_checkudata(_L, -1, "QMetaObject"));
        const QMetaObject* mo = ud->mo;
        lua_pop(_L, 1);
        return mo;
    }

    bool isFunction() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isfunction(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    LuaFunction toFunction() const {
        if (!isFunction()) return LuaFunction{ _L, LUA_NOREF };
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        int funcRef = luaL_ref(_L, LUA_REGISTRYINDEX);
        return LuaFunction{ _L, funcRef };
    }

    bool isThread() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isthread(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isTable() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_istable(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    void setTableField(const char* key, const QLuaValue& value) const {
        if (!isTable()) return;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        value.pushToStack();
        lua_setfield(_L, -2, key);
        lua_pop(_L, 1); // pop the original table
    }

    void deleteTableField(const char* key) const {
        if (!isTable()) return;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        lua_pushnil(_L);
        lua_setfield(_L, -2, key);
        lua_pop(_L, 1); // pop the original table
    }

    void pushToStack() const {
        if (_ref == LUA_NOREF) {
            lua_pushnil(_L);
        }
        else {
            lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        }
    }

    bool isLightUserData() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_islightuserdata(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

private:
    lua_State* _L;
    int _ref;
};