#pragma once

#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QMetaMethod>
#include <QtCore/QVariantList>
#include "LuaHelper.h"

class LuaSlotProxy: public QObject {
public:
    LuaSlotProxy(LuaFunction callback, QMetaMethod m, QObject* parent = nullptr):
        QObject(parent),
        _callback(callback),
        _m(m) {
        lua_rawgeti(_callback.L, LUA_REGISTRYINDEX, _callback.ref);
        _callback.ref = luaL_ref(_callback.L, LUA_REGISTRYINDEX);
    }

    ~LuaSlotProxy() {
        if (_callback.ref != LUA_NOREF) {
            luaL_unref(_callback.L, LUA_REGISTRYINDEX, _callback.ref);
        }
    }

    int qt_metacall(QMetaObject::Call call, int methodId, void** a) override {
        methodId = QObject::qt_metacall(call, methodId, a);
        if (methodId < 0)
            return methodId;

        if (call == QMetaObject::InvokeMetaMethod) {
            if (methodId == 0) {
                QVariantList args;
                for (int i = 0; i < _m.parameterCount(); ++i) {
                    int typeId = _m.parameterType(i);
                    args.append(QVariant::fromMetaType(QMetaType(typeId), a[i + 1]));
                }
                _callback.call(args);
            }
            --methodId;
        }
        return methodId;
    }

private:
    LuaFunction _callback;
    QMetaMethod _m;
};