#pragma once

#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaMethod>
#include <LuaHelper.h>
#include <LuaFunction.h>

class QLuaMethod: public QObject {
public:
    QLuaMethod(LuaFunction callback, QMetaMethod m, QObject* parent = nullptr);

    ~QLuaMethod();

    int qt_metacall(QMetaObject::Call call, int methodId, void** a) override;

private:
    LuaFunction _callback;
    QMetaMethod _m;
};